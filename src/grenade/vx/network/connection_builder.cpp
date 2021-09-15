#include "grenade/vx/network/connection_builder.h"

#include "halco/hicann-dls/vx/v2/event.h"
#include "halco/hicann-dls/vx/v2/padi.h"
#include "halco/hicann-dls/vx/v2/routing_crossbar.h"
#include "halco/hicann-dls/vx/v2/synapse.h"
#include "halco/hicann-dls/vx/v2/synapse_driver.h"
#include "hate/algorithm.h"
#include "hate/variant.h"
#include <map>
#include <set>
#include <unordered_set>
#include <boost/range/adaptor/reversed.hpp>

namespace grenade::vx::network {

using namespace halco::hicann_dls::vx::v2;
using namespace halco::common;

bool requires_routing(std::shared_ptr<Network> const& current, std::shared_ptr<Network> const& old)
{
	assert(current);
	assert(old);

	// check if populations changed
	if (current->populations != old->populations) {
		return true;
	}
	// check if projection count changed
	if (current->projections.size() != old->projections.size()) {
		return true;
	}
	// check if projection topology changed
	for (auto const& [descriptor, projection] : current->projections) {
		auto const& old_projection = old->projections.at(descriptor);
		if ((projection.population_pre != old_projection.population_pre) ||
		    (projection.population_post != old_projection.population_post) ||
		    (projection.receptor_type != old_projection.receptor_type) ||
		    (projection.connections.size() != old_projection.connections.size())) {
			return true;
		}
		for (size_t i = 0; i < projection.connections.size(); ++i) {
			auto const& connection = projection.connections.at(i);
			auto const& old_connection = projection.connections.at(i);
			if ((connection.index_pre != old_connection.index_pre) ||
			    (connection.index_post != old_connection.index_post)) {
				return true;
			}
		}
	}
	// check if MADC recording was added or removed
	if (static_cast<bool>(current->madc_recording) != static_cast<bool>(old->madc_recording)) {
		return true;
	}
	return false;
}

namespace {

/**
 * Parity to make neurons in both hemispheres distinguishable.
 * Instead of choosing neuron.toNeuronRowOnDLS() as parity, which would look like
 * 0 0
 * 1 1
 * we chose equal parity for top-left and bottom-right, as well as top-right and bottom-left like:
 * 0 1
 * 1 0
 * This results in ability to utilize all synapse drivers with neurons from a single hemisphere,
 * which for example is a requirement when connecting all-to-all within all neurons from a
 * hemisphere.
 */
bool get_parity(AtomicNeuronOnDLS const neuron)
{
	return neuron.toNeuronRowOnDLS().value() != neuron.toNeuronColumnOnDLS()
	                                                .toNeuronEventOutputOnDLS()
	                                                .toNeuronBackendConfigBlockOnDLS();
}

bool get_parity(SynapseDriverOnDLS const syndrv)
{
	return syndrv.toSynapseDriverOnSynapseDriverBlock().toSynapseDriverOnPADIBus() % 2;
}

} // namespace

ConnectionBuilder::ConnectionBuilder()
{
	m_used_padi_rows_even.fill(0);
	m_used_padi_rows_odd.fill(0);
}

std::vector<ConnectionBuilder::UsedSynapseRow> const& ConnectionBuilder::get_used_synapse_rows()
    const
{
	return m_used_synapse_rows_internal;
}

ConnectionBuilder::UsedSynapseRow const& ConnectionBuilder::get_synapse_row(
    halco::hicann_dls::vx::v2::AtomicNeuronOnDLS const pre,
    halco::hicann_dls::vx::v2::AtomicNeuronOnDLS const post,
    Projection::ReceptorType const receptor_type)
{
	// calculate crossbar output, PADI bus
	// neurons per crossbar input channel
	auto const crossbar_input =
	    pre.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS().toCrossbarInputOnDLS();
	auto const padi_bus_block = post.toNeuronRowOnDLS().toPADIBusBlockOnDLS();
	PADIBusOnPADIBusBlock const padi_bus_on_block(
	    crossbar_input.toEnum() % PADIBusOnPADIBusBlock::size);
	PADIBusOnDLS const padi_bus(padi_bus_on_block, padi_bus_block);

	// check if a synapse driver for the presynaptic neuron's padi bus with given
	// receptor type was already allocated and if its synapse to the
	// postsynaptic neuron is still available
	for (auto& used_synapse_row : m_used_synapse_rows_internal) {
		PADIBusOnDLS const used_padi_bus(
		    used_synapse_row.synapse_driver.toSynapseDriverOnSynapseDriverBlock()
		        .toPADIBusOnPADIBusBlock(),
		    used_synapse_row.synapse_driver.toSynapseDriverBlockOnDLS().toPADIBusBlockOnDLS());
		auto const matches_parity =
		    (get_parity(pre) == get_parity(used_synapse_row.synapse_driver));
		if (matches_parity && (used_padi_bus == padi_bus) &&
		    (used_synapse_row.receptor_type == receptor_type) &&
		    !used_synapse_row.neurons_post.contains(post)) {
			used_synapse_row.neurons_post.insert(post);
			return used_synapse_row;
		}
	}

	// check if there is a driver available
	auto& padi_row_index =
	    (get_parity(pre) ? m_used_padi_rows_odd : m_used_padi_rows_even).at(padi_bus);
	if (padi_row_index >=
	    SynapseDriverOnPADIBus::size * SynapseRowOnSynapseDriver::size / NeuronRowOnDLS::size) {
		throw std::runtime_error(
		    "Too many connections. Try decreasing the weight values in a way they do not exceed "
		    "large multiples of the maximum synaptic weight (63) or reduce the number of "
		    "projections with the same presynaptic population, but different receptor types.");
	}

	// set synapse driver row address compare mask
	auto const syndrv(
	    SynapseDriverOnPADIBus(
	        (padi_row_index / SynapseRowOnSynapseDriver::size) * NeuronRowOnDLS::size +
	        get_parity(pre))
	        .toSynapseDriverOnSynapseDriverBlock()[padi_bus.toPADIBusOnPADIBusBlock()]);
	SynapseDriverOnDLS const global_syndrv(
	    syndrv, padi_bus.toPADIBusBlockOnDLS().toSynapseDriverBlockOnDLS());

	m_synapse_driver_compare_mask[global_syndrv] =
	    haldls::vx::v2::SynapseDriverConfig::RowAddressCompareMask(0b00000);

	UsedSynapseRow const used_synapse_row{
	    global_syndrv,
	    SynapseRowOnSynapseDriver(padi_row_index % SynapseRowOnSynapseDriver::size),
	    {post},
	    receptor_type};
	m_used_synapse_rows_internal.push_back(used_synapse_row);

	padi_row_index++;

	return m_used_synapse_rows_internal.back();
}

ConnectionBuilder::Result::PlacedConnection ConnectionBuilder::get_placed_connection(
    halco::hicann_dls::vx::v2::AtomicNeuronOnDLS pre,
    halco::hicann_dls::vx::v2::AtomicNeuronOnDLS post,
    UsedSynapseRow const& synapse_row,
    Projection::Connection const& connection) const
{
	// neurons per crossbar input channel
	constexpr size_t neuron_columns_per_channel =
	    NeuronColumnOnDLS::size / NeuronEventOutputOnDLS::size;
	// uniquely identify a neuron column on a event output channel
	size_t const neuron_on_channel = pre.toNeuronColumnOnDLS() % neuron_columns_per_channel;
	// uniquely identify a neuron on a event output channel block
	size_t const neuron_channel_block =
	    pre.toNeuronColumnOnDLS() / (NeuronColumnOnDLS::size / NeuronBackendConfigBlockOnDLS::size);
	// add placed weight and label data
	lola::vx::v2::SynapseMatrix::Label const label(
	    neuron_on_channel + (neuron_channel_block * neuron_columns_per_channel));
	SynapseRowOnDLS const row(
	    synapse_row.synapse_driver.toSynapseDriverOnSynapseDriverBlock().toSynapseRowOnSynram().at(
	        synapse_row.synapse_row),
	    synapse_row.synapse_driver.toSynapseDriverBlockOnDLS().toSynramOnDLS());
	SynapseOnSynapseRow const synapse = post.toNeuronColumnOnDLS().toSynapseOnSynapseRow();

	return {connection.weight, label, row, synapse};
}

ConnectionBuilder::Result::PlacedConnection ConnectionBuilder::get_placed_connection(
    std::pair<PopulationDescriptor, size_t> pre,
    halco::hicann_dls::vx::v2::AtomicNeuronOnDLS post,
    SynapseDriverOnSynapseDriverBlock const& synapse_driver,
    Projection::Connection const& connection) const
{
	auto const& used_synapse_driver =
	    m_used_synapse_drivers_external.at(post.toNeuronRowOnDLS().toHemisphereOnDLS())
	        .at(synapse_driver);
	// add placed weight and label data
	auto const pres_it =
	    std::find(used_synapse_driver.pres.begin(), used_synapse_driver.pres.end(), pre);
	lola::vx::v2::SynapseMatrix::Label const label(
	    std::distance(used_synapse_driver.pres.begin(), pres_it));
	// check from the end of the rows if the column is used, because we fill from the beginning
	// and thereby get the latest addition
	for (auto const& [r, used_row] : boost::adaptors::reverse(used_synapse_driver.rows)) {
		if (used_row.neurons_post.contains(post.toNeuronColumnOnDLS())) {
			SynapseRowOnDLS const row(
			    synapse_driver.toSynapseRowOnSynram().at(r),
			    post.toNeuronRowOnDLS().toSynramOnDLS());
			SynapseOnSynapseRow const synapse = post.toNeuronColumnOnDLS().toSynapseOnSynapseRow();
			if (connection.weight > lola::vx::v2::SynapseMatrix::Weight::max) {
				throw std::out_of_range(
				    "Single synaptic weight larger than hardware range not supported.");
			}
			return {lola::vx::v2::SynapseMatrix::Weight(connection.weight), label, row, synapse};
		}
	}
	throw std::logic_error("Supplied synapse driver row does not have post-synaptic neuron free.");
}

void ConnectionBuilder::calculate_free_for_external_synapse_drivers()
{
	size_t free_drivers = 0;
	for (auto const padibus : iter_all<PADIBusOnDLS>()) {
		auto used = std::max(m_used_padi_rows_odd.at(padibus), m_used_padi_rows_even.at(padibus));
		// Higher PADI bus on block is given for non-zero NeuronEventOutputOnDLS.
		// This results in bits [8,9,10] being filled with its value and therefore restriction on
		// the allowed mask. Since if any NeuronBackendBlockOnDLS(1) source is present, bit 10 needs
		// to match, it is always disabled. Example:
		// - NeuronColumnOnDLS(32) is located on NeuronEventOutputOnDLS(1)
		// -> bit 8 is set in 14-bit crossbar label and 11-bit PADI-label (crossbar: 00000100000000)
		// -> equals bit 2 in 5-bit RowAddressCompareMask, therefore until this bit the mask is to
		//    be zero (synapse driver: 00100xxxxxx, x is forwarded to synapses)
		// -> calculation below can be used when setting 4 < used <=8
		// -> for NeuronEventOutputOnNeuronBackendBlock(2,3) the calculation below can be used for
		//    8 < used <= 16, since bit 9 (and maybe 8) is set: (crossbar: 00001?00000000, synapse
		//    driver: 01?00xxxxxx)
		auto const bus_on_block = padibus.toPADIBusOnPADIBusBlock();
		if (used) {
			if (bus_on_block.value() == 1) {
				used = std::max(used, static_cast<size_t>(8));
			} else if (bus_on_block.value() > 1) {
				used = std::max(used, static_cast<size_t>(16));
			}
		}

		haldls::vx::v2::SynapseDriverConfig::RowAddressCompareMask config;
		if (used == 0) {
			free_drivers = SynapseDriverOnPADIBus::size;
			config = haldls::vx::v2::SynapseDriverConfig::RowAddressCompareMask(0b11111);
		} else if (used <= 4) {
			free_drivers = 12;
			config = haldls::vx::v2::SynapseDriverConfig::RowAddressCompareMask(0b01101);
		} else if (used <= 8) {
			free_drivers = 8;
			config = haldls::vx::v2::SynapseDriverConfig::RowAddressCompareMask(0b01001);
		} else if (used <= 16) {
			free_drivers = 0;
			config = haldls::vx::v2::SynapseDriverConfig::RowAddressCompareMask(0b00001);
		} else {
			free_drivers = 0;
			config = haldls::vx::v2::SynapseDriverConfig::RowAddressCompareMask(0b00001);
		}

		// The free synapse drivers are the ones with higher index on PADI-bus
		auto const start_syndrv = SynapseDriverOnPADIBus::size - free_drivers;
		for (size_t driver = 0; driver < free_drivers; ++driver) {
			m_free_for_external_synapse_drivers[padibus.toPADIBusBlockOnDLS().toHemisphereOnDLS()]
			    .push_back(SynapseDriverOnSynapseDriverBlock(
			        SynapseDriverOnPADIBus(start_syndrv + driver)
			            .toSynapseDriverOnSynapseDriverBlock()[padibus.toPADIBusOnPADIBusBlock()]));
		}
		for (auto const local_driver : iter_all<SynapseDriverOnPADIBus>()) {
			SynapseDriverOnDLS const driver(
			    local_driver
			        .toSynapseDriverOnSynapseDriverBlock()[padibus.toPADIBusOnPADIBusBlock()],
			    padibus.toPADIBusBlockOnDLS().toSynapseDriverBlockOnDLS());
			m_synapse_driver_compare_mask[driver] = config;
		}
	}
}

halco::hicann_dls::vx::v2::SynapseDriverOnSynapseDriverBlock ConnectionBuilder::get_synapse_driver(
    std::pair<PopulationDescriptor, size_t> const& pre,
    std::set<halco::hicann_dls::vx::v2::NeuronColumnOnDLS> const& post,
    HemisphereOnDLS const& hemisphere,
    Projection::ReceptorType const receptor_type)
{
	auto const add_neuron = [](auto& used_synapse_driver, auto const p) {
		for (auto& [_, used_row] : used_synapse_driver.rows) {
			if (!used_row.neurons_post.contains(p)) {
				used_row.neurons_post.insert(p);
				return;
			}
		}
	};
	auto const add_neurons = [add_neuron, post](auto& used_synapse_driver) {
		for (auto const p : post) {
			add_neuron(used_synapse_driver, p);
		}
	};
	// first check if a already used synapse driver with matching receptor type has all requested
	// post neurons free and still has label-space to support a additional presynaptic partner
	for (auto& [c, used_synapse_driver] : m_used_synapse_drivers_external[hemisphere]) {
		// synapse driver has to have less than label-range many presynaptic partners
		if (used_synapse_driver.pres.size() >= lola::vx::v2::SynapseMatrix::Label::size) {
			continue;
		}
		std::map<NeuronColumnOnDLS, std::set<SynapseRowOnSynapseDriver>> used_neurons;
		for (auto const& [r, row] : used_synapse_driver.rows) {
			// receptor type has to match
			if (row.receptor_type != receptor_type) {
				for (auto const n : iter_all<NeuronColumnOnDLS>()) {
					used_neurons[n].insert(r);
				}
				continue;
			}
			for (auto const& n : row.neurons_post) {
				used_neurons[n].insert(r);
			}
		}
		std::set<NeuronColumnOnDLS> used_columns;
		for (auto const& [n, rs] : used_neurons) {
			if (rs.size() == used_synapse_driver.rows.size()) {
				used_columns.insert(n);
			}
		}
		// all requested post neurons have to be free in one row
		if (hate::has_intersection(used_columns, post)) {
			continue;
		}
		used_synapse_driver.pres.push_back(pre);
		add_neurons(used_synapse_driver);
		return c;
	}

	// find and create new used synapse row
	// first check if there is a synapse driver which features a unused row
	if (auto driver = std::find_if(
	        m_used_synapse_drivers_external[hemisphere].begin(),
	        m_used_synapse_drivers_external[hemisphere].end(),
	        [&](auto const& used_driver) {
		        assert(!used_driver.second.rows.empty());
		        return (used_driver.second.rows.size() < SynapseRowOnSynapseDriver::size) &&
		               (used_driver.second.pres.size() < lola::vx::v2::SynapseMatrix::Label::size);
	        });
	    driver != m_used_synapse_drivers_external[hemisphere].end()) {
		std::vector<SynapseRowOnSynapseDriver> added_rows;
		for (auto const r : iter_all<SynapseRowOnSynapseDriver>()) {
			if (!driver->second.rows.contains(r)) {
				driver->second.rows.insert({r, {{}, receptor_type}});
				added_rows.push_back(r);
			}
		}
		assert(added_rows.size() == 1);
		driver->second.pres.push_back(pre);
		add_neurons(driver->second);
		return driver->first;
	}
	// add new synapse driver
	if (m_free_for_external_synapse_drivers.at(hemisphere).empty()) {
		throw std::runtime_error("No more synapse drivers free for external event input.");
	}
	auto const synapse_driver = m_free_for_external_synapse_drivers.at(hemisphere).front();
	m_free_for_external_synapse_drivers.at(hemisphere).pop_front();
	m_used_synapse_drivers_external[hemisphere].insert(
	    {synapse_driver, {{pre}, {{SynapseRowOnSynapseDriver(), {{}, receptor_type}}}}});
	m_synapse_driver_compare_mask[SynapseDriverOnDLS(
	    synapse_driver, hemisphere.toSynapseDriverBlockOnDLS())] =
	    haldls::vx::v2::SynapseDriverConfig::RowAddressCompareMask(0b11111);
	add_neurons(m_used_synapse_drivers_external.at(hemisphere).at(synapse_driver));
	return synapse_driver;
}

RoutingResult build_routing(std::shared_ptr<Network> const& network)
{
	ConnectionBuilder builder;

	assert(network);

	// check that no connection between equal ends is contained in multiple projections
	for (auto const& [descriptor, projection] : network->projections) {
		auto const get_unique_connections = [](auto const& connections) {
			std::set<std::pair<size_t, size_t>> unique_connections;
			for (auto const& c : connections) {
				unique_connections.insert({c.index_pre, c.index_post});
			}
			return unique_connections;
		};
		auto const has_same_edge = [descriptor, projection,
		                            get_unique_connections](auto const& other) {
			if (other.first == descriptor) { // ignore same projection
				return false;
			}
			return (projection.receptor_type == other.second.receptor_type) &&
			       (projection.population_pre == other.second.population_pre) &&
			       (projection.population_post == other.second.population_post) &&
			       hate::has_intersection(
			           get_unique_connections(projection.connections),
			           get_unique_connections(other.second.connections));
		};
		if (std::any_of(network->projections.begin(), network->projections.end(), has_same_edge)) {
			throw std::runtime_error(
			    "Projection features same single connection than other projection.");
		}
	}

	RoutingResult result;

	// first on-chip connections are placed
	auto& placed_connections = result.connections;
	for (auto const& [p, projection] : network->projections) {
		if (!std::holds_alternative<Population>(
		        network->populations.at(projection.population_pre))) {
			continue;
		}
		if (projection.connections.empty()) {
			placed_connections.emplace(
			    std::make_pair(p, std::vector<std::vector<RoutingResult::PlacedConnection>>{}));
			continue;
		}
		auto const& population_pre =
		    std::get<Population>(network->populations.at(projection.population_pre));
		auto const& population_post =
		    std::get<Population>(network->populations.at(projection.population_post));
		for (size_t i = 0; i < projection.connections.size(); ++i) {
			// acquire synapse row to use
			auto const pre = population_pre.neurons.at(projection.connections.at(i).index_pre);
			auto const post = population_post.neurons.at(projection.connections.at(i).index_post);

			auto const& synapse_row = builder.get_synapse_row(pre, post, projection.receptor_type);

			auto const placed_connection =
			    builder.get_placed_connection(pre, post, synapse_row, projection.connections.at(i));

			placed_connections[p].push_back({placed_connection});
		}
	}

	// then from-off-chip connections are placed
	// get_synapse_driver needs all post-synaptic neurons of one receptor type, therefore we order
	// by it here
	std::map<
	    Projection::ReceptorType, std::map<
	                                  std::pair<PopulationDescriptor, size_t>,
	                                  std::map<NeuronRowOnDLS, std::set<NeuronColumnOnDLS>>>>
	    connections_external_unplaced;
	for (auto const& [p, projection] : network->projections) {
		if (!std::holds_alternative<ExternalPopulation>(
		        network->populations.at(projection.population_pre))) {
			continue;
		}
		if (projection.connections.empty()) {
			placed_connections.emplace(
			    std::make_pair(p, std::vector<std::vector<RoutingResult::PlacedConnection>>{}));
			continue;
		}
		auto const& population_post =
		    std::get<Population>(network->populations.at(projection.population_post));

		for (size_t i = 0; i < projection.connections.size(); ++i) {
			// acquire synapse row to use
			auto const index_pre = projection.connections.at(i).index_pre;
			auto const post = population_post.neurons.at(projection.connections.at(i).index_post);
			auto const [_, success] =
			    connections_external_unplaced[projection.receptor_type][std::pair(
			        projection.population_pre, index_pre)][post.toNeuronRowOnDLS()]
			        .insert(post.toNeuronColumnOnDLS());
			assert(success);
		}
	}

	builder.calculate_free_for_external_synapse_drivers();

	// get synapse driver masks
	result.synapse_driver_compare_masks = builder.m_synapse_driver_compare_mask;

	for (auto const& [receptor_type, p] : connections_external_unplaced) {
		for (auto const& [pre, nrns] : p) {
			for (auto const& [row, neurons] : nrns) {
				auto const synapse_driver = builder.get_synapse_driver(
				    pre, neurons, row.toHemisphereOnDLS(), receptor_type);
				for (auto const& [j, projection] : network->projections) {
					if ((projection.population_pre == pre.first) &&
					    (projection.receptor_type == receptor_type)) {
						auto const& population_post = std::get<Population>(
						    network->populations.at(projection.population_post));
						for (size_t i = 0; i < projection.connections.size(); ++i) {
							auto const& connection = projection.connections.at(i);
							auto const post = population_post.neurons.at(connection.index_post);
							if ((connection.index_pre == pre.second) &&
							    (post.toNeuronRowOnDLS() == row)) {
								auto const post = population_post.neurons.at(connection.index_post);
								assert(post.toNeuronRowOnDLS() == row);
								auto const placed_connection = builder.get_placed_connection(
								    pre, post, synapse_driver, connection);
								placed_connections[j].push_back({placed_connection});
							}
						}
					}
				}
			}
		}
	}

	// get synapse row modes
	auto& synapse_row_modes = result.synapse_row_modes;
	auto const get_row_mode = [](auto const receptor_type) {
		if (receptor_type == Projection::ReceptorType::excitatory) {
			return haldls::vx::v2::SynapseDriverConfig::RowMode::excitatory;
		} else {
			return haldls::vx::v2::SynapseDriverConfig::RowMode::inhibitory;
		}
	};
	// TODO: only disable all rows of used synapse drivers
	for (auto const synapse_row : iter_all<SynapseRowOnDLS>()) {
		synapse_row_modes[synapse_row] = haldls::vx::v2::SynapseDriverConfig::RowMode::disabled;
	}
	for (auto const& used_row : builder.m_used_synapse_rows_internal) {
		auto const synapse_row = SynapseRowOnDLS(
		    used_row.synapse_driver.toSynapseDriverOnSynapseDriverBlock()
		        .toSynapseRowOnSynram()[used_row.synapse_row],
		    used_row.synapse_driver.toSynapseDriverBlockOnDLS().toSynramOnDLS());
		synapse_row_modes[synapse_row] = get_row_mode(used_row.receptor_type);
	}
	for (auto const& [hemisphere, used_drivers] : builder.m_used_synapse_drivers_external) {
		for (auto const& [syndrv, used_driver] : used_drivers) {
			for (auto const& [r, row] : used_driver.rows) {
				auto const synapse_row =
				    SynapseRowOnDLS(syndrv.toSynapseRowOnSynram()[r], hemisphere.toSynramOnDLS());
				synapse_row_modes[synapse_row] = get_row_mode(row.receptor_type);
			}
		}
	}

	// get neuron output labels
	auto& internal_neuron_labels = result.internal_neuron_labels;
	for (auto const& [descriptor, population] : network->populations) {
		if (!std::holds_alternative<Population>(population)) {
			continue;
		}
		auto const& pop = std::get<Population>(population);
		auto& local_neuron_labels = internal_neuron_labels[descriptor];
		for (auto const& neuron : pop.neurons) {
			// neurons per crossbar input channel
			constexpr size_t neuron_columns_per_channel =
			    NeuronColumnOnDLS::size / NeuronEventOutputOnDLS::size;
			// uniquely identify a neuron column on a event output channel
			size_t const neuron_on_channel =
			    neuron.toNeuronColumnOnDLS() % neuron_columns_per_channel;
			// uniquely identify a neuron on a event output channel block
			size_t const neuron_channel_block =
			    neuron.toNeuronColumnOnDLS() /
			    (NeuronColumnOnDLS::size / NeuronBackendConfigBlockOnDLS::size);
			// uniquely identify a neuron column in the lower 6-bit -> at a synapse.
			// This leads to full usage possibility of the top neurons
			haldls::vx::v2::NeuronBackendConfig::AddressOut label(
			    (get_parity(neuron) * NeuronColumnOnDLS::size /
			     NeuronEventOutputOnNeuronBackendBlock::size) +
			    neuron_on_channel + (neuron_channel_block * neuron_columns_per_channel));
			local_neuron_labels.push_back(label);
		}
	}

	// get external populations' spike labels
	auto& external_spike_labels = result.external_spike_labels;
	for (auto const& [descriptor, population] : network->populations) {
		if (!std::holds_alternative<ExternalPopulation>(population)) {
			continue;
		}
		auto const& pop = std::get<ExternalPopulation>(population);
		external_spike_labels[descriptor].resize(pop.size);
	}
	for (auto const& [hemisphere, used_drivers] : builder.m_used_synapse_drivers_external) {
		for (auto const& [syndrv, used_driver] : used_drivers) {
			auto const syndrv_on_padi_bus = syndrv.toSynapseDriverOnPADIBus();
			auto const padi_bus_on_block = syndrv.toPADIBusOnPADIBusBlock();
			haldls::vx::v2::SpikeLabel label;
			label.set_neuron_label(NeuronLabel(hemisphere.value() << 13));
			label.set_spl1_address(SPL1Address(padi_bus_on_block));
			label.set_row_select_address(
			    haldls::vx::v2::PADIEvent::RowSelectAddress(syndrv_on_padi_bus.value()));
			for (size_t addr = 0; addr < used_driver.pres.size(); ++addr) {
				label.set_synapse_label(lola::vx::v2::SynapseMatrix::Label(addr));
				auto const& index = used_driver.pres.at(addr);
				external_spike_labels.at(index.first).at(index.second).push_back(label);
			}
		}
	}

	// calculate crossbar node config
	auto& crossbar_nodes = result.crossbar_nodes;
	// enable recurrent connections
	for (auto const cinput : iter_all<NeuronEventOutputOnDLS>()) {
		CrossbarNodeOnDLS const coord(
		    CrossbarOutputOnDLS(cinput % PADIBusOnPADIBusBlock::size),
		    cinput.toCrossbarInputOnDLS());
		crossbar_nodes[coord] = haldls::vx::v2::CrossbarNode();
	}
	for (auto const cinput : iter_all<NeuronEventOutputOnDLS>()) {
		CrossbarNodeOnDLS const coord(
		    CrossbarOutputOnDLS(cinput % PADIBusOnPADIBusBlock::size + PADIBusOnPADIBusBlock::size),
		    cinput.toCrossbarInputOnDLS());
		crossbar_nodes[coord] = haldls::vx::v2::CrossbarNode();
	}
	// enable L2 output
	for (auto const cinput : iter_all<NeuronEventOutputOnDLS>()) {
		CrossbarNodeOnDLS const coord(
		    CrossbarOutputOnDLS(PADIBusOnDLS::size + (cinput % PADIBusOnPADIBusBlock::size)),
		    cinput.toCrossbarInputOnDLS());
		crossbar_nodes[coord] = haldls::vx::v2::CrossbarNode();
	}
	// disable non-diagonal input from L2
	for (auto const cinput : iter_all<SPL1Address>()) {
		for (auto const coutput : iter_all<PADIBusOnDLS>()) {
			CrossbarNodeOnDLS const coord(
			    coutput.toCrossbarOutputOnDLS(), cinput.toCrossbarInputOnDLS());
			crossbar_nodes[coord] = haldls::vx::v2::CrossbarNode::drop_all;
		}
	}
	// enable input from L2 to top half
	for (auto const coutput : iter_all<PADIBusOnPADIBusBlock>()) {
		CrossbarNodeOnDLS const coord(
		    PADIBusOnDLS(coutput, PADIBusBlockOnDLS::top).toCrossbarOutputOnDLS(),
		    SPL1Address(coutput).toCrossbarInputOnDLS());
		crossbar_nodes[coord] = haldls::vx::v2::CrossbarNode();
	}
	// enable input from L2 to bottom half
	for (auto const coutput : iter_all<PADIBusOnPADIBusBlock>()) {
		CrossbarNodeOnDLS const coord(
		    PADIBusOnDLS(coutput, PADIBusBlockOnDLS::bottom).toCrossbarOutputOnDLS(),
		    SPL1Address(coutput).toCrossbarInputOnDLS());
		crossbar_nodes[coord] = haldls::vx::v2::CrossbarNode();
	}

	return result;
}

} // namespace grenade::vx::network
