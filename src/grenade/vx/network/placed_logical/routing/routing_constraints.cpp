#include "grenade/vx/network/placed_logical/routing/routing_constraints.h"

#include "grenade/vx/network/placed_logical/network.h"
#include "hate/math.h"
#include <iostream>

namespace grenade::vx::network::placed_logical::routing {

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;

RoutingConstraints::RoutingConstraints(
    Network const& network, ConnectionRoutingResult const& connection_routing_result) :
    m_network(network), m_connection_routing_result(connection_routing_result)
{}

void RoutingConstraints::check() const
{
	using namespace halco::hicann_dls::vx::v3;

	auto const neuron_in_degree = get_neuron_in_degree();
	if (*std::max_element(neuron_in_degree.begin(), neuron_in_degree.end()) >
	    SynapseRowOnSynram::size) {
		throw std::runtime_error(
		    "Neuron(s) with in-degree larger than the number of synapse rows exist.");
	}

	auto const neuron_in_degree_per_padi_bus = get_neuron_in_degree_per_padi_bus();
	for (auto const& in_degree_per_padi_bus : neuron_in_degree_per_padi_bus) {
		for (auto const& in_degree : in_degree_per_padi_bus) {
			if (in_degree > SynapseDriverOnPADIBus::size * SynapseRowOnSynapseDriver::size) {
				throw std::runtime_error("Neuron(s) with in-degree on a single PADI-bus larger "
				                         "than its number of synapse rows exist.");
			}
		}
	}

	auto const num_synapse_rows_per_padi_bus = get_num_synapse_rows_per_padi_bus();
	if (*std::max_element(
	        num_synapse_rows_per_padi_bus.begin(), num_synapse_rows_per_padi_bus.end()) >
	    SynapseDriverOnPADIBus::size * SynapseRowOnSynapseDriver::size) {
		throw std::runtime_error(
		    "PADI-bus(ses) with required number of synapse rows larger than the "
		    "number of synapse rows on a single PADI-bus exist.");
	}
}

halco::hicann_dls::vx::v3::PADIBusOnDLS RoutingConstraints::InternalConnection::toPADIBusOnDLS()
    const
{
	auto const source_event_output = source.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS();
	auto const crossbar_input = source_event_output.toCrossbarInputOnDLS();

	auto const target_padi_bus_block = target.toNeuronRowOnDLS().toPADIBusBlockOnDLS();

	using namespace halco::hicann_dls::vx::v3;
	return PADIBusOnDLS(
	    PADIBusOnPADIBusBlock(crossbar_input % PADIBusOnPADIBusBlock::size), target_padi_bus_block);
}

bool RoutingConstraints::InternalConnection::operator==(InternalConnection const& other) const
{
	return source == other.source && target == other.target && descriptor == other.descriptor &&
	       receptor_type == other.receptor_type;
}

bool RoutingConstraints::InternalConnection::operator!=(InternalConnection const& other) const
{
	return !(*this == other);
}

std::vector<RoutingConstraints::InternalConnection> RoutingConstraints::get_internal_connections()
    const
{
	std::vector<InternalConnection> ret;
	for (auto const& [descriptor, projection] : m_network.projections) {
		if (!std::holds_alternative<Population>(
		        m_network.populations.at(projection.population_pre)) ||
		    !std::holds_alternative<Population>(
		        m_network.populations.at(projection.population_post))) {
			continue;
		}
		auto const& population_pre =
		    std::get<Population>(m_network.populations.at(projection.population_pre));
		auto const& population_post =
		    std::get<Population>(m_network.populations.at(projection.population_post));
		size_t i = 0;
		for (auto const& connection : projection.connections) {
			auto const& spike_master_pre = population_pre.neurons.at(connection.index_pre.first)
			                                   .compartments.at(connection.index_pre.second)
			                                   .spike_master;
			assert(spike_master_pre);
			for (auto const& an_target : m_connection_routing_result.at(descriptor)
			                                 .at(i)
			                                 .atomic_neurons_on_target_compartment) {
				ret.push_back(
				    {population_pre.neurons.at(connection.index_pre.first)
				         .coordinate.get_placed_compartments()
				         .at(connection.index_pre.second)
				         .at(spike_master_pre->neuron_on_compartment),
				     population_post.neurons.at(connection.index_post.first)
				         .coordinate.get_placed_compartments()
				         .at(connection.index_post.second)
				         .at(an_target),
				     {descriptor, i},
				     projection.receptor.type});
			}
			i++;
		}
	}
	return ret;
}

halco::hicann_dls::vx::v3::PADIBusOnDLS RoutingConstraints::BackgroundConnection::toPADIBusOnDLS()
    const
{
	return source.toPADIBusOnDLS();
}

bool RoutingConstraints::BackgroundConnection::operator==(BackgroundConnection const& other) const
{
	return source == other.source && target == other.target && descriptor == other.descriptor &&
	       receptor_type == other.receptor_type;
}

bool RoutingConstraints::BackgroundConnection::operator!=(BackgroundConnection const& other) const
{
	return !(*this == other);
}

std::vector<RoutingConstraints::BackgroundConnection>
RoutingConstraints::get_background_connections() const
{
	std::vector<BackgroundConnection> ret;
	for (auto const& [descriptor, projection] : m_network.projections) {
		if (!std::holds_alternative<BackgroundSpikeSourcePopulation>(
		        m_network.populations.at(projection.population_pre)) ||
		    !std::holds_alternative<Population>(
		        m_network.populations.at(projection.population_post))) {
			continue;
		}
		auto const& population_pre = std::get<BackgroundSpikeSourcePopulation>(
		    m_network.populations.at(projection.population_pre));
		auto const& population_post =
		    std::get<Population>(m_network.populations.at(projection.population_post));
		size_t i = 0;
		for (auto const& connection : projection.connections) {
			for (auto const& an_target : m_connection_routing_result.at(descriptor)
			                                 .at(i)
			                                 .atomic_neurons_on_target_compartment) {
				ret.push_back(
				    {BackgroundSpikeSourceOnDLS(
				         population_pre.coordinate.at(
				             population_post.neurons.at(connection.index_post.first)
				                 .coordinate.get_placed_compartments()
				                 .at(connection.index_post.second)
				                 .at(an_target)
				                 .toNeuronRowOnDLS()
				                 .toHemisphereOnDLS()) +
				         population_post.neurons.at(connection.index_post.first)
				                 .coordinate.get_placed_compartments()
				                 .at(connection.index_post.second)
				                 .at(an_target)
				                 .toNeuronRowOnDLS()
				                 .toHemisphereOnDLS() *
				             PADIBusOnPADIBusBlock::size),
				     population_post.neurons.at(connection.index_post.first)
				         .coordinate.get_placed_compartments()
				         .at(connection.index_post.second)
				         .at(an_target),
				     {descriptor, i},
				     projection.receptor.type});
			}
			i++;
		}
	}
	return ret;
}

halco::hicann_dls::vx::v3::PADIBusBlockOnDLS
RoutingConstraints::ExternalConnection::toPADIBusBlockOnDLS() const
{
	return target.toNeuronRowOnDLS().toPADIBusBlockOnDLS();
}

bool RoutingConstraints::ExternalConnection::operator==(ExternalConnection const& other) const
{
	return target == other.target && descriptor == other.descriptor &&
	       receptor_type == other.receptor_type;
}

bool RoutingConstraints::ExternalConnection::operator!=(ExternalConnection const& other) const
{
	return !(*this == other);
}

std::vector<RoutingConstraints::ExternalConnection> RoutingConstraints::get_external_connections()
    const
{
	std::vector<ExternalConnection> ret;
	for (auto const& [descriptor, projection] : m_network.projections) {
		if (!std::holds_alternative<ExternalPopulation>(
		        m_network.populations.at(projection.population_pre))) {
			continue;
		}
		auto const& population_post =
		    std::get<Population>(m_network.populations.at(projection.population_post));
		size_t i = 0;
		for (auto const& connection : projection.connections) {
			for (auto const& an_target : m_connection_routing_result.at(descriptor)
			                                 .at(i)
			                                 .atomic_neurons_on_target_compartment) {
				ret.push_back(
				    {population_post.neurons.at(connection.index_post.first)
				         .coordinate.get_placed_compartments()
				         .at(connection.index_post.second)
				         .at(an_target),
				     {descriptor, i},
				     projection.receptor.type});
			}
			i++;
		}
	}
	return ret;
}

halco::common::typed_array<
    std::map<Receptor::Type, std::vector<std::pair<ProjectionDescriptor, size_t>>>,
    halco::hicann_dls::vx::v3::HemisphereOnDLS>
RoutingConstraints::get_external_connections_per_hemisphere() const
{
	halco::common::typed_array<
	    std::map<Receptor::Type, std::vector<std::pair<ProjectionDescriptor, size_t>>>,
	    halco::hicann_dls::vx::v3::HemisphereOnDLS>
	    ret;

	for (auto const& connection : get_external_connections()) {
		ret[connection.target.toNeuronRowOnDLS().toHemisphereOnDLS()][connection.receptor_type]
		    .push_back(connection.descriptor);
	}
	return ret;
}

halco::common::typed_array<
    std::map<Receptor::Type, std::set<std::pair<PopulationDescriptor, size_t>>>,
    halco::hicann_dls::vx::v3::HemisphereOnDLS>
RoutingConstraints::get_external_sources_to_hemisphere() const
{
	auto const external_connections_per_hemisphere = get_external_connections_per_hemisphere();

	halco::common::typed_array<
	    std::map<Receptor::Type, std::set<std::pair<PopulationDescriptor, size_t>>>,
	    halco::hicann_dls::vx::v3::HemisphereOnDLS>
	    ret;

	for (auto const hemisphere :
	     halco::common::iter_all<halco::hicann_dls::vx::v3::HemisphereOnDLS>()) {
		for (auto const& [receptor_type, descriptors] :
		     external_connections_per_hemisphere[hemisphere]) {
			for (auto const& descriptor : descriptors) {
				auto const& projection = m_network.projections.at(descriptor.first);
				auto const& connection = projection.connections.at(descriptor.second);
				ret[hemisphere][receptor_type].insert(
				    {projection.population_pre, connection.index_pre.first});
			}
		}
	}
	return ret;
}

halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>
RoutingConstraints::get_neuron_in_degree() const
{
	halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> in_degree;
	in_degree.fill(0);

	for (auto const& connection : get_internal_connections()) {
		in_degree[connection.target]++;
	}
	for (auto const& connection : get_background_connections()) {
		in_degree[connection.target]++;
	}
	for (auto const& connection : get_external_connections()) {
		in_degree[connection.target]++;
	}
	return in_degree;
}

halco::common::typed_array<
    halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::PADIBusOnPADIBusBlock>,
    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>
RoutingConstraints::get_neuron_in_degree_per_padi_bus() const
{
	halco::common::typed_array<
	    halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::PADIBusOnPADIBusBlock>,
	    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>
	    in_degree;
	{
		halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::PADIBusOnPADIBusBlock> zero;
		zero.fill(0);
		in_degree.fill(zero);
	}
	for (auto const& connection : get_internal_connections()) {
		in_degree[connection.target][connection.toPADIBusOnDLS().toPADIBusOnPADIBusBlock()]++;
	}
	for (auto const& connection : get_background_connections()) {
		in_degree[connection.target][connection.toPADIBusOnDLS().toPADIBusOnPADIBusBlock()]++;
	}
	return in_degree;
}

halco::common::
    typed_array<std::map<Receptor::Type, size_t>, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>
    RoutingConstraints::get_neuron_in_degree_per_receptor_type() const
{
	halco::common::typed_array<
	    std::map<Receptor::Type, size_t>, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>
	    in_degree;
	for (auto const& connection : get_internal_connections()) {
		in_degree[connection.target][connection.receptor_type]++;
	}
	for (auto const& connection : get_background_connections()) {
		in_degree[connection.target][connection.receptor_type]++;
	}
	for (auto const& connection : get_external_connections()) {
		in_degree[connection.target][connection.receptor_type]++;
	}
	return in_degree;
}

halco::common::typed_array<
    halco::common::typed_array<
        std::map<Receptor::Type, size_t>,
        halco::hicann_dls::vx::v3::PADIBusOnPADIBusBlock>,
    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>
RoutingConstraints::get_neuron_in_degree_per_receptor_type_per_padi_bus() const
{
	halco::common::typed_array<
	    halco::common::typed_array<
	        std::map<Receptor::Type, size_t>, halco::hicann_dls::vx::v3::PADIBusOnPADIBusBlock>,
	    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>
	    in_degree;
	for (auto const& connection : get_internal_connections()) {
		in_degree[connection.target][connection.toPADIBusOnDLS().toPADIBusOnPADIBusBlock()]
		         [connection.receptor_type]++;
	}
	for (auto const& connection : get_background_connections()) {
		in_degree[connection.target][connection.toPADIBusOnDLS().toPADIBusOnPADIBusBlock()]
		         [connection.receptor_type]++;
	}
	return in_degree;
}

halco::common::
    typed_array<std::map<Receptor::Type, size_t>, halco::hicann_dls::vx::v3::PADIBusOnDLS>
    RoutingConstraints::get_num_synapse_rows_per_padi_bus_per_receptor_type() const
{
	halco::common::typed_array<
	    std::map<Receptor::Type, size_t>, halco::hicann_dls::vx::v3::PADIBusOnDLS>
	    num;

	auto const neuron_in_degree = get_neuron_in_degree_per_receptor_type_per_padi_bus();

	using namespace halco::hicann_dls::vx::v3;
	for (auto const nrn : halco::common::iter_all<AtomicNeuronOnDLS>()) {
		for (auto const padi_bus : halco::common::iter_all<PADIBusOnPADIBusBlock>()) {
			PADIBusOnDLS const padi_bus_on_dls(
			    padi_bus, nrn.toNeuronRowOnDLS().toPADIBusBlockOnDLS());
			for (auto const& [r, c] : neuron_in_degree[nrn][padi_bus]) {
				if (!num[padi_bus_on_dls].contains(r)) {
					num[padi_bus_on_dls][r] = 0;
				}
				num.at(padi_bus_on_dls).at(r) = std::max(num.at(padi_bus_on_dls).at(r), c);
			}
		}
	}
	return num;
}

halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::PADIBusOnDLS>
RoutingConstraints::get_num_synapse_rows_per_padi_bus() const
{
	halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::PADIBusOnDLS> num;
	num.fill(0);

	auto const num_synapse_rows_per_receptor_type =
	    get_num_synapse_rows_per_padi_bus_per_receptor_type();

	using namespace halco::hicann_dls::vx::v3;
	for (auto const padi_bus : halco::common::iter_all<PADIBusOnDLS>()) {
		for (auto const& [_, c] : num_synapse_rows_per_receptor_type[padi_bus]) {
			num.at(padi_bus) += c;
		}
	}
	return num;
}

std::map<
    halco::hicann_dls::vx::v3::NeuronEventOutputOnDLS,
    std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>>
RoutingConstraints::get_neurons_on_event_output() const
{
	std::map<
	    halco::hicann_dls::vx::v3::NeuronEventOutputOnDLS,
	    std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>>
	    ret;

	std::set<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> neurons;
	using namespace halco::hicann_dls::vx::v3;
	for (auto const& [_, pop] : m_network.populations) {
		if (!std::holds_alternative<Population>(pop)) {
			continue;
		}
		auto const& population = std::get<Population>(pop);
		for (size_t i = 0; i < population.neurons.size(); ++i) {
			for (auto const& [compartment, atomic_neurons] :
			     population.neurons.at(i).coordinate.get_placed_compartments()) {
				auto const& spike_master =
				    population.neurons.at(i).compartments.at(compartment).spike_master;
				if (spike_master && spike_master->enable_record_spikes) {
					neurons.insert(atomic_neurons.at(spike_master->neuron_on_compartment));
				}
			}
		}
	}

	for (auto const& neuron : neurons) {
		ret[neuron.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS()].push_back(neuron);
	}

	return ret;
}

std::set<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>
RoutingConstraints::get_neither_recorded_nor_source_neurons() const
{
	std::set<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> ret;

	using namespace halco::hicann_dls::vx::v3;
	for (auto const& [_, pop] : m_network.populations) {
		if (!std::holds_alternative<Population>(pop)) {
			continue;
		}
		auto const& population = std::get<Population>(pop);
		for (size_t i = 0; i < population.neurons.size(); ++i) {
			for (auto const& [compartment, atomic_neurons] :
			     population.neurons.at(i).coordinate.get_placed_compartments()) {
				auto const& spike_master =
				    population.neurons.at(i).compartments.at(compartment).spike_master;
				if (!spike_master || (spike_master && !spike_master->enable_record_spikes)) {
					for (auto const& atomic_neuron : atomic_neurons) {
						ret.insert(atomic_neuron);
					}
				} else if (spike_master && spike_master->enable_record_spikes) {
					for (auto const& atomic_neuron : atomic_neurons) {
						if (atomic_neuron !=
						    atomic_neurons.at(spike_master->neuron_on_compartment)) {
							ret.insert(atomic_neuron);
						}
					}
				}
			}
		}
	}

	return ret;
}

std::map<
    halco::hicann_dls::vx::v3::PADIBusOnDLS,
    std::set<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>>
RoutingConstraints::get_neurons_on_padi_bus() const
{
	std::map<
	    halco::hicann_dls::vx::v3::PADIBusOnDLS,
	    std::set<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>>
	    ret;

	for (auto const& connection : get_internal_connections()) {
		ret[connection.toPADIBusOnDLS()].insert(connection.source);
	}

	return ret;
}

halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::PADIBusOnDLS>
RoutingConstraints::get_num_background_sources_on_padi_bus() const
{
	halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::PADIBusOnDLS> ret;
	ret.fill(0);

	for (auto const& [_, pop] : m_network.populations) {
		if (!std::holds_alternative<BackgroundSpikeSourcePopulation>(pop)) {
			continue;
		}
		auto const& population = std::get<BackgroundSpikeSourcePopulation>(pop);
		for (auto const& [hemisphere, bus] : population.coordinate) {
			ret[PADIBusOnDLS(bus, hemisphere.toPADIBusBlockOnDLS())] = population.size;
		}
	}

	return ret;
}

std::map<
    halco::hicann_dls::vx::v3::PADIBusOnDLS,
    std::set<halco::hicann_dls::vx::v3::NeuronEventOutputOnDLS>>
RoutingConstraints::get_neuron_event_outputs_on_padi_bus() const
{
	auto const internal_connections = get_internal_connections();
	std::map<
	    halco::hicann_dls::vx::v3::PADIBusOnDLS,
	    std::set<halco::hicann_dls::vx::v3::NeuronEventOutputOnDLS>>
	    ret;
	for (auto const& connection : internal_connections) {
		ret[connection.toPADIBusOnDLS()].insert(
		    connection.source.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS());
	}
	return ret;
}

halco::common::
    typed_array<RoutingConstraints::PADIBusConstraints, halco::hicann_dls::vx::v3::PADIBusOnDLS>
    RoutingConstraints::get_padi_bus_constraints() const
{
	typed_array<PADIBusConstraints, PADIBusOnDLS> padi_bus_constraints;

	auto const num_background_sources_on_padi_bus = get_num_background_sources_on_padi_bus();
	auto const neurons_on_padi_bus = get_neurons_on_padi_bus();
	auto const neurons_on_event_output = get_neurons_on_event_output();
	auto const internal_connections = get_internal_connections();
	auto const background_connections = get_background_connections();

	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		auto& constraints = padi_bus_constraints[padi_bus];

		constraints.num_background_spike_sources = num_background_sources_on_padi_bus[padi_bus];

		for (auto const& connection : internal_connections) {
			if (connection.toPADIBusOnDLS() == padi_bus) {
				constraints.internal_connections.push_back(connection);
			}
		}
		for (auto const& connection : background_connections) {
			if (connection.toPADIBusOnDLS() == padi_bus) {
				constraints.background_connections.push_back(connection);
			}
		}

		if (neurons_on_padi_bus.contains(padi_bus)) {
			auto const& neuron_sources = neurons_on_padi_bus.at(padi_bus);
			constraints.neuron_sources = {neuron_sources.begin(), neuron_sources.end()};
		}

		for (auto const neuron_event_output_block : iter_all<NeuronBackendConfigBlockOnDLS>()) {
			NeuronEventOutputOnDLS neuron_event_output(
			    NeuronEventOutputOnNeuronBackendBlock(padi_bus.value()), neuron_event_output_block);
			if (neurons_on_event_output.contains(neuron_event_output)) {
				auto const& neurons = neurons_on_event_output.at(neuron_event_output);
				constraints.only_recorded_neurons.insert(neurons.begin(), neurons.end());
			}
		}
		for (auto const& neuron : constraints.neuron_sources) {
			constraints.only_recorded_neurons.erase(neuron);
		}
	}

	return padi_bus_constraints;
}

} // namespace grenade::vx::routing::network
