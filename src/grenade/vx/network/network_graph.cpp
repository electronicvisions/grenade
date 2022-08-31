#include "grenade/vx/network/network_graph.h"

#include "grenade/vx/network/connectum.h"
#include "grenade/vx/network/exception.h"
#include "hate/indent.h"
#include "hate/join.h"
#include "hate/math.h"
#include "hate/variant.h"
#include <bit>
#include <set>
#include <log4cxx/logger.h>

namespace grenade::vx::network {

std::shared_ptr<Network> const& NetworkGraph::get_network() const
{
	return m_network;
}

Graph const& NetworkGraph::get_graph() const
{
	return m_graph;
}

std::optional<Graph::vertex_descriptor> NetworkGraph::get_event_input_vertex() const
{
	return m_event_input_vertex;
}

std::optional<Graph::vertex_descriptor> NetworkGraph::get_event_output_vertex() const
{
	return m_event_output_vertex;
}

std::optional<Graph::vertex_descriptor> NetworkGraph::get_madc_sample_output_vertex() const
{
	return m_madc_sample_output_vertex;
}

std::vector<Graph::vertex_descriptor> NetworkGraph::get_cadc_sample_output_vertex() const
{
	return m_cadc_sample_output_vertex;
}

NetworkGraph::SpikeLabels const& NetworkGraph::get_spike_labels() const
{
	return m_spike_labels;
}

bool NetworkGraph::valid() const
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;

	auto logger = log4cxx::Logger::getLogger("grenade.NetworkGraph.valid()");
	assert(m_network);

	// check that for each neuron/source in every population an event-label is known
	for (auto const& [descriptor, population] : m_network->populations) {
		if (!m_spike_labels.contains(descriptor)) {
			LOG4CXX_ERROR(logger, "No spike-labels for population(" << descriptor << ") present.");
			return false;
		}
		auto const get_size = hate::overloaded(
		    [](Population const& p) { return p.neurons.size(); },
		    [](ExternalPopulation const& p) { return p.size; },
		    [](BackgroundSpikeSourcePopulation const& p) { return p.size; });
		auto const size = m_spike_labels.at(descriptor).size();
		auto const expected_size = std::visit(get_size, population);
		if (size != expected_size) {
			LOG4CXX_ERROR(
			    logger, "Not the correct number of spike-labels for population("
			                << descriptor << ") present: actual(" << size
			                << ") instead of expected(" << expected_size << ").");
			return false;
		}
	}

	// check that event labels of recorded neurons are unique
	{
		std::set<SpikeLabel> unique;
		for (auto const& [descriptor, population] : m_network->populations) {
			if (!std::holds_alternative<Population>(population)) {
				continue;
			}
			auto const& pop = std::get<Population>(population);
			for (size_t i = 0; i < pop.neurons.size(); ++i) {
				if (!pop.enable_record_spikes.at(i)) {
					continue;
				}
				auto const& label = m_spike_labels.at(descriptor).at(i).at(0);
				if (label) {
					auto const& [_, success] = unique.insert(*label);
					if (!success) {
						LOG4CXX_ERROR(
						    logger, "Label of to be recorded neuron at(" << descriptor << ", " << i
						                                                 << ") is unique.");
						return false;
					}
				}
			}
		}
	}

	// check that all on-chip populations are represented
	for (auto const& [descriptor, population] : m_network->populations) {
		if (!std::holds_alternative<Population>(population)) {
			continue;
		}
		if (!m_neuron_vertices.contains(descriptor)) {
			LOG4CXX_ERROR(
			    logger, "No hardware graph vertex available for population(" << descriptor << ").");
			return false;
		}
		auto const& pop = std::get<Population>(population);
		std::set<AtomicNeuronOnDLS> abstract_network_neurons(
		    pop.neurons.begin(), pop.neurons.end());
		std::set<AtomicNeuronOnDLS> hardware_network_neurons;
		for (auto const& [hemisphere, vertex_descriptor] : m_neuron_vertices.at(descriptor)) {
			auto const& vertex =
			    std::get<vertex::NeuronView>(m_graph.get_vertex_property(vertex_descriptor));
			auto const row = vertex.get_row();
			if (row.toHemisphereOnDLS() != hemisphere) {
				LOG4CXX_ERROR(
				    logger,
				    "NeuronView vertex descriptor hemisphere key does not match vertex property.");
				return false;
			}
			for (auto const& column : vertex.get_columns()) {
				AtomicNeuronOnDLS const neuron(column, row);
				hardware_network_neurons.insert(neuron);
			}
		}
		if (abstract_network_neurons != hardware_network_neurons) {
			LOG4CXX_ERROR(
			    logger, "Abstract network population("
			                << descriptor
			                << ") neurons don't match hardware network NeuronView's neurons.");
			return false;
		}
	}

	// check that all on-chip neurons feature correct event labels
	for (auto const& [descriptor, population] : m_network->populations) {
		if (!std::holds_alternative<Population>(population)) {
			continue;
		}
		auto const& pop = std::get<Population>(population);
		for (auto const& [_, vertex_descriptor] : m_neuron_vertices.at(descriptor)) {
			auto const& vertex =
			    std::get<vertex::NeuronView>(m_graph.get_vertex_property(vertex_descriptor));
			auto const row = vertex.get_row();
			auto const& configs = vertex.get_configs();
			auto const& columns = vertex.get_columns();
			for (size_t i = 0; i < configs.size(); ++i) {
				AtomicNeuronOnDLS const neuron(columns.at(i), row);
				auto const index = std::distance(
				    pop.neurons.begin(), std::find(pop.neurons.begin(), pop.neurons.end(), neuron));
				auto const& expected_label = m_spike_labels.at(descriptor).at(index).at(0);
				if (static_cast<bool>(expected_label) != static_cast<bool>(configs.at(i).label)) {
					LOG4CXX_ERROR(
					    logger, "Abstract network on-chip neuron("
					                << std::boolalpha << descriptor << ", " << i
					                << ") event label (" << static_cast<bool>(expected_label)
					                << ") does not match event output status (enabled/disabled) in "
					                   "hardware "
					                   "network NeuronView's config("
					                << static_cast<bool>(configs.at(i).label) << ").");
					return false;
				}
				if (expected_label) {
					SpikeLabel actual_label;
					actual_label.set_neuron_backend_address_out(*(configs.at(i).label));
					actual_label.set_neuron_event_output(
					    neuron.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS());
					actual_label.set_spl1_address(
					    SPL1Address(neuron.toNeuronColumnOnDLS()
					                    .toNeuronEventOutputOnDLS()
					                    .toNeuronEventOutputOnNeuronBackendBlock()));
					if (actual_label != *expected_label) {
						LOG4CXX_ERROR(
						    logger, "Abstract network on-chip neuron("
						                << descriptor << ", " << i << ") event label ("
						                << *expected_label
						                << ") does not match event label in hardware "
						                   "network NeuronView's config("
						                << actual_label << ").");
						return false;
					}
				}
			}
		}
	}

	// check that all on-chip background spike sources are represented
	for (auto const& [descriptor, population] : m_network->populations) {
		if (!std::holds_alternative<BackgroundSpikeSourcePopulation>(population)) {
			continue;
		}
		auto const& pop = std::get<BackgroundSpikeSourcePopulation>(population);
		if (!m_background_spike_source_vertices.contains(descriptor)) {
			LOG4CXX_ERROR(
			    logger, "No hardware network vertices for on-chip background spike source("
			                << descriptor << ").");
			return false;
		}
		for (auto const& [hemisphere, padi_bus] : pop.coordinate) {
			if (!m_background_spike_source_vertices.at(descriptor).contains(hemisphere)) {
				LOG4CXX_ERROR(
				    logger, "No hardware network vertex for on-chip background spike source("
				                << descriptor << ") on " << hemisphere << ".");
				return false;
			}
			auto const& vertex =
			    std::get<vertex::BackgroundSpikeSource>(m_graph.get_vertex_property(
			        m_background_spike_source_vertices.at(descriptor).at(hemisphere)));
			if (vertex.get_coordinate().toPADIBusOnDLS() !=
			    PADIBusOnDLS(pop.coordinate.at(hemisphere), hemisphere.toPADIBusBlockOnDLS())) {
				LOG4CXX_ERROR(
				    logger,
				    "On-chip background spike source location does not match between abstract "
				    "network ("
				        << PADIBusOnDLS(
				               pop.coordinate.at(hemisphere), hemisphere.toPADIBusBlockOnDLS())
				        << " and hardware network (" << vertex.get_coordinate().toPADIBusOnDLS()
				        << ").");
				return false;
			}
			if (vertex.get_config().get_enable_random() != pop.config.enable_random) {
				LOG4CXX_ERROR(
				    logger, "On-chip background spike source enable_random does not match between "
				            "abstract network ("
				                << pop.config.enable_random << " and hardware network ("
				                << vertex.get_config().get_enable_random() << ").");
				return false;
			}
			if (pop.config.enable_random) {
				if (hate::math::pow(2, std::popcount(vertex.get_config().get_mask().value())) !=
				    pop.size) {
					LOG4CXX_ERROR(
					    logger, "On-chip background spike source size does not match between "
					            "abstract network ("
					                << pop.size << " and hardware network ("
					                << hate::math::pow(
					                       2, std::popcount(vertex.get_config().get_mask().value()))
					                << ").");
					return false;
				}
			}
			if (vertex.get_config().get_rate() != pop.config.rate) {
				LOG4CXX_ERROR(
				    logger,
				    "On-chip background spike source rate does not match between abstract network ("
				        << pop.config.rate << " and hardware network ("
				        << vertex.get_config().get_rate() << ").");
				return false;
			}
			if (vertex.get_config().get_seed() != pop.config.seed) {
				LOG4CXX_ERROR(
				    logger,
				    "On-chip background spike source seed does not match between abstract network ("
				        << pop.config.seed << " and hardware network ("
				        << vertex.get_config().get_seed() << ").");
				return false;
			}
		}
	}

	// check that all on-chip background spike sources feature correct event labels
	for (auto const& [descriptor, population] : m_network->populations) {
		if (!std::holds_alternative<BackgroundSpikeSourcePopulation>(population)) {
			continue;
		}
		auto const& pop = std::get<BackgroundSpikeSourcePopulation>(population);
		size_t label_entry = 0;
		for (auto const& [hemisphere, padi_bus] : pop.coordinate) {
			auto const& vertex =
			    std::get<vertex::BackgroundSpikeSource>(m_graph.get_vertex_property(
			        m_background_spike_source_vertices.at(descriptor).at(hemisphere)));
			for (size_t i = 0; i < pop.size; ++i) {
				auto expected_label = m_spike_labels.at(descriptor).at(i).at(label_entry);
				if (!expected_label) {
					LOG4CXX_ERROR(
					    logger, "Abstract network on-chip background source("
					                << descriptor << ", " << i
					                << ") event label can't be disabled.");
					return false;
				}
				expected_label->set_neuron_label(NeuronLabel(
				    expected_label->get_neuron_label() & ~vertex.get_config().get_mask()));
				SpikeLabel actual_label;
				actual_label.set_neuron_label(NeuronLabel(
				    vertex.get_config().get_neuron_label() & ~vertex.get_config().get_mask()));
				if (actual_label != *expected_label) {
					LOG4CXX_ERROR(
					    logger, "Abstract network on-chip background source("
					                << descriptor << ", " << i << ") event label ("
					                << *expected_label
					                << ") does not match event label in hardware "
					                   "network BackgroundSpikeSource's config("
					                << actual_label << ").");
					return false;
				}
			}
			label_entry++;
		}
	}

	// check that all projections are present, their weight, receptor type and post-synaptic neuron
	// matches
	for (auto const& [descriptor, projection] : m_network->projections) {
		if (!m_synapse_vertices.contains(descriptor)) {
			LOG4CXX_ERROR(
			    logger, "Abstract network projection(" << descriptor
			                                           << ") not represented in hardware network.");
			return false;
		}
		auto const& neurons_post =
		    std::get<Population>(m_network->populations.at(projection.population_post)).neurons;
		for (auto const& [hemisphere, vertex_descriptor] : m_synapse_vertices.at(descriptor)) {
			// calculate synapse row receptor types
			std::map<SynapseRowOnSynram, Projection::ReceptorType> receptor_type_per_row;
			for (auto const in_edge : boost::make_iterator_range(
			         boost::in_edges(vertex_descriptor, m_graph.get_graph()))) {
				auto const& synapse_driver = std::get<vertex::SynapseDriver>(
				    m_graph.get_vertex_property(boost::source(in_edge, m_graph.get_graph())));
				auto const& coordinate = synapse_driver.get_coordinate();
				auto const& config = synapse_driver.get_config();
				for (auto const row_on_synapse_driver : iter_all<SynapseRowOnSynapseDriver>()) {
					auto const row = coordinate.toSynapseRowOnSynram()[row_on_synapse_driver];
					switch (config.row_modes[row_on_synapse_driver]) {
						case SynapseDriverConfig::RowMode::excitatory: {
							receptor_type_per_row[row] = Projection::ReceptorType::excitatory;
							break;
						}
						case SynapseDriverConfig::RowMode::inhibitory: {
							receptor_type_per_row[row] = Projection::ReceptorType::inhibitory;
							break;
						}
						case SynapseDriverConfig::RowMode::excitatory_and_inhibitory: {
							LOG4CXX_ERROR(
							    logger, "SynapseDriver in hardware network features "
							            "excitatory_and_inhibitory row mode.");
							return false;
						}
						case SynapseDriverConfig::RowMode::disabled: {
							break;
						}
						default: {
							throw std::logic_error("Synapse driver row mode not supported.");
						}
					}
				}
			}
			auto const& synapse_array_view = std::get<vertex::SynapseArrayViewSparse>(
			    m_graph.get_vertex_property(vertex_descriptor));
			if (synapse_array_view.get_synram().toHemisphereOnDLS() != hemisphere) {
				LOG4CXX_ERROR(
				    logger, "SynapseArrayView vertex descriptor hemisphere key does not match "
				            "vertex property.");
				return false;
			}
			auto const neuron_row = synapse_array_view.get_synram().toNeuronRowOnDLS();
			vertex::SynapseArrayViewSparse::Synapses const synapses(
			    synapse_array_view.get_synapses().begin(), synapse_array_view.get_synapses().end());
			vertex::SynapseArrayViewSparse::Columns const columns(
			    synapse_array_view.get_columns().begin(), synapse_array_view.get_columns().end());
			vertex::SynapseArrayViewSparse::Rows const rows(
			    synapse_array_view.get_rows().begin(), synapse_array_view.get_rows().end());
			size_t index_on_hemisphere = 0;
			for (size_t i = 0; i < projection.connections.size(); ++i) {
				auto const& connection = projection.connections.at(i);
				if (neurons_post.at(connection.index_post).toNeuronRowOnDLS() == neuron_row) {
					auto const& synapse = synapses.at(index_on_hemisphere);
					if (synapse.weight != connection.weight) {
						LOG4CXX_ERROR(
						    logger, "Abstract network synaptic weight does not match hardware "
						            "network synaptic weight.");
						return false;
					}
					if (columns.at(synapse.index_column).toNeuronColumnOnDLS() !=
					    neurons_post.at(connection.index_post).toNeuronColumnOnDLS()) {
						LOG4CXX_ERROR(
						    logger, "Abstract network post-synaptic neuron does not match hardware "
						            "network post-synaptic neuron.");
						return false;
					}
					if (receptor_type_per_row.at(rows.at(synapse.index_row)) !=
					    projection.receptor_type) {
						LOG4CXX_ERROR(
						    logger, "Abstract network receptor type does not match hardware "
						            "network receptor type.");
						return false;
					}
					index_on_hemisphere++;
				}
			}
		}
	}

	// TODO: check recorded neurons can be recorded via crossbar and event outputs

	// check that connectum of hardware network matches expected connectum of abstract network
	try {
		auto const connectum_from_abstract_network =
		    generate_connectum_from_abstract_network(*this);
		auto const connectum_from_hardware_network =
		    generate_connectum_from_hardware_network(*this);
		if ((connectum_from_abstract_network.size() != connectum_from_hardware_network.size()) ||
		    !std::is_permutation(
		        connectum_from_abstract_network.begin(), connectum_from_abstract_network.end(),
		        connectum_from_hardware_network.begin())) {
			std::vector<ConnectumConnection> missing_in_hardware_network;
			for (auto const& connection : connectum_from_abstract_network) {
				if (std::find(
				        connectum_from_hardware_network.begin(),
				        connectum_from_hardware_network.end(),
				        connection) == connectum_from_hardware_network.end()) {
					missing_in_hardware_network.push_back(connection);
				}
			}
			std::vector<ConnectumConnection> missing_in_abstract_network;
			for (auto const& connection : connectum_from_hardware_network) {
				if (std::find(
				        connectum_from_abstract_network.begin(),
				        connectum_from_abstract_network.end(),
				        connection) == connectum_from_abstract_network.end()) {
					missing_in_abstract_network.push_back(connection);
				}
			}
			LOG4CXX_ERROR(
			    logger, "Abstract network connectum (size: "
			                << connectum_from_abstract_network.size()
			                << ") does not match hardware "
			                   "network connectum (size: "
			                << connectum_from_hardware_network.size() << "):\n\tabstract network:\n"
			                << hate::indent(
			                       hate::join_string(
			                           connectum_from_abstract_network.begin(),
			                           connectum_from_abstract_network.end(), "\n"),
			                       "\t\t")
			                << "\n\thardware network:\n"
			                << hate::indent(
			                       hate::join_string(
			                           connectum_from_hardware_network.begin(),
			                           connectum_from_hardware_network.end(), "\n"),
			                       "\t\t"));
			return false;
		}
	} catch (InvalidNetworkGraph const& error) {
		LOG4CXX_ERROR(logger, "Error during generation of connectum to validate: " << error.what());
		return false;
	}

	// check that is required dense in order is fulfilled
	for (auto const& [_, plasticity_rule] : m_network->plasticity_rules) {
		if (!plasticity_rule.enable_requires_one_source_per_row_in_order) {
			continue;
		}
		for (auto const& descriptor : plasticity_rule.projections) {
			for (auto const& [hemisphere, vertex_descriptor] : m_synapse_vertices.at(descriptor)) {
				auto const& synapse_array_view = std::get<vertex::SynapseArrayViewSparse>(
				    m_graph.get_vertex_property(vertex_descriptor));
				std::vector<std::pair<
				    halco::hicann_dls::vx::v3::SynapseRowOnSynram,
				    halco::hicann_dls::vx::v3::SynapseOnSynapseRow>>
				    locations;
				auto const rows = synapse_array_view.get_rows();
				auto const columns = synapse_array_view.get_columns();
				if (rows.size() * columns.size() != synapse_array_view.get_synapses().size()) {
					LOG4CXX_ERROR(logger, "SynapseArrayView is not dense.");
					return false;
				}
				for (auto const& synapse : synapse_array_view.get_synapses()) {
					locations.push_back({rows[synapse.index_row], columns[synapse.index_column]});
				}
				if (!std::is_sorted(locations.begin(), locations.end())) {
					LOG4CXX_ERROR(logger, "SynapseArrayView is not in order.");
					return false;
				}
			}
		}
	}

	return true;
}

std::ostream& operator<<(std::ostream& os, NetworkGraph::PlacedConnection const& placed_connection)
{
	os << "PlacedConnection(\n\t" << placed_connection.weight << "\n\t"
	   << placed_connection.synapse_row << "\n\t" << placed_connection.synapse_on_row << "\n)";
	return os;
}

NetworkGraph::PlacedConnection NetworkGraph::get_placed_connection(
    ProjectionDescriptor const descriptor, size_t const index) const
{
	assert(m_network);
	auto const& projection = m_network->projections.at(descriptor);
	auto const index_post = projection.connections.at(index).index_post;
	auto const& neurons =
	    std::get<Population>(m_network->populations.at(projection.population_post)).neurons;

	auto const hemisphere = neurons.at(index_post).toNeuronRowOnDLS().toHemisphereOnDLS();
	size_t index_on_hemisphere = 0;
	for (size_t i = 0; i < index; ++i) {
		auto const i_post = projection.connections.at(i).index_post;
		index_on_hemisphere +=
		    (neurons.at(i_post).toNeuronRowOnDLS().toHemisphereOnDLS() == hemisphere);
	}

	auto const vertex_descriptor = m_synapse_vertices.at(descriptor).at(hemisphere);
	auto const& synapse_array_view_sparse =
	    std::get<vertex::SynapseArrayViewSparse>(m_graph.get_vertex_property(vertex_descriptor));
	assert(synapse_array_view_sparse.get_synram().toHemisphereOnDLS() == hemisphere);

	auto const rows = synapse_array_view_sparse.get_rows();
	auto const columns = synapse_array_view_sparse.get_columns();
	auto const synapses = synapse_array_view_sparse.get_synapses();

	assert(index_on_hemisphere < synapses.size());
	auto const& synapse = synapses[index_on_hemisphere];
	assert(synapse.index_row < rows.size());
	assert(synapse.index_column < columns.size());
	PlacedConnection placed_connection{
	    synapse.weight,
	    halco::hicann_dls::vx::v3::SynapseRowOnDLS(
	        rows[synapse.index_row], synapse_array_view_sparse.get_synram()),
	    columns[synapse.index_column]};
	return placed_connection;
}

NetworkGraph::PlacedConnections NetworkGraph::get_placed_connections(
    ProjectionDescriptor const descriptor) const
{
	PlacedConnections placed_connections;
	assert(m_network);
	for (size_t i = 0; i < m_network->projections.at(descriptor).connections.size(); ++i) {
		placed_connections.push_back(get_placed_connection(descriptor, i));
	}
	return placed_connections;
}

} // namespace grenade::vx::network
