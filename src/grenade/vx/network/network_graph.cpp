#include "grenade/vx/network/network_graph.h"

#include "grenade/vx/network/connectum.h"
#include "grenade/vx/network/exception.h"
#include "hate/indent.h"
#include "hate/join.h"
#include "hate/variant.h"
#include <sstream>
#include <log4cxx/logger.h>

namespace grenade::vx::network {

std::shared_ptr<Network> const& NetworkGraph::get_network() const
{
	return m_network;
}

signal_flow::Graph const& NetworkGraph::get_graph() const
{
	return m_graph;
}

NetworkGraph::GraphTranslation const& NetworkGraph::get_graph_translation() const
{
	return m_graph_translation;
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
		if (!m_graph_translation.spike_labels.contains(descriptor)) {
			LOG4CXX_ERROR(logger, "No spike-labels for population(" << descriptor << ") present.");
			return false;
		}
		auto const get_size = hate::overloaded(
		    [](Population const& p) { return p.neurons.size(); },
		    [](ExternalSourcePopulation const& p) { return p.size; },
		    [](BackgroundSourcePopulation const& p) { return p.size; });
		auto const size = m_graph_translation.spike_labels.at(descriptor).size();
		auto const expected_size = std::visit(get_size, population);
		if (size != expected_size) {
			LOG4CXX_ERROR(
			    logger, "Not the correct number of spike-labels for population("
			                << descriptor << ") present: actual(" << size
			                << ") instead of expected(" << expected_size << ").");
			return false;
		}
		auto const check_compartments = hate::overloaded(
		    [this, descriptor, logger](Population const& p) {
			    for (size_t n = 0; n < p.neurons.size(); ++n) {
				    for (auto const& [compartment, ans] :
				         p.neurons.at(n).coordinate.get_placed_compartments()) {
					    if (!m_graph_translation.spike_labels.at(descriptor)
					             .at(n)
					             .contains(compartment)) {
						    LOG4CXX_ERROR(
						        logger, "No spike-labels for compartment ("
						                    << compartment << ") on neuron (" << n
						                    << ") on internal population(" << descriptor
						                    << ") present.");
						    return false;
					    }
				    }
			    }
			    return true;
		    },
		    [this, descriptor, logger](ExternalSourcePopulation const& p) {
			    for (size_t n = 0; n < p.size; ++n) {
				    if (!m_graph_translation.spike_labels.at(descriptor)
				             .at(n)
				             .contains(CompartmentOnLogicalNeuron())) {
					    LOG4CXX_ERROR(
					        logger, "No spike-labels for neuron ("
					                    << n << ") on external population(" << descriptor
					                    << ") present.");
					    return false;
				    }
			    }
			    return true;
		    },
		    [this, descriptor, logger](BackgroundSourcePopulation const& p) {
			    for (size_t n = 0; n < p.size; ++n) {
				    if (!m_graph_translation.spike_labels.at(descriptor)
				             .at(n)
				             .contains(CompartmentOnLogicalNeuron())) {
					    LOG4CXX_ERROR(
					        logger, "No spike-labels for neuron ("
					                    << n << ") on background population(" << descriptor
					                    << ") present.");
					    return false;
				    }
			    }
			    return true;
		    });
		auto const has_expected_compartments = std::visit(check_compartments, population);
		if (!has_expected_compartments) {
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
				for (auto const& [compartment_on_neuron, compartment] :
				     pop.neurons.at(i).compartments) {
					if (!compartment.spike_master ||
					    !compartment.spike_master->enable_record_spikes) {
						continue;
					}
					auto const& label = m_graph_translation.spike_labels.at(descriptor)
					                        .at(i)
					                        .at(compartment_on_neuron)
					                        .at(compartment.spike_master->neuron_on_compartment);
					if (label) {
						auto const& [_, success] = unique.insert(*label);
						if (!success) {
							LOG4CXX_ERROR(
							    logger, "Label of to be recorded neuron at("
							                << descriptor << ", " << i << ", "
							                << compartment_on_neuron << ") is not unique.");
							return false;
						}
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
		if (!m_graph_translation.neuron_vertices.contains(descriptor)) {
			LOG4CXX_ERROR(
			    logger, "No hardware graph vertex available for population(" << descriptor << ").");
			return false;
		}
		auto const& pop = std::get<Population>(population);
		std::set<AtomicNeuronOnDLS> abstract_network_neurons;
		for (auto const& neuron : pop.neurons) {
			for (auto const& an : neuron.coordinate.get_atomic_neurons()) {
				abstract_network_neurons.insert(an);
			}
		}
		std::set<AtomicNeuronOnDLS> hardware_network_neurons;
		for (auto const& [hemisphere, vertex_descriptor] :
		     m_graph_translation.neuron_vertices.at(descriptor)) {
			auto const& vertex = std::get<signal_flow::vertex::NeuronView>(
			    m_graph.get_vertex_property(vertex_descriptor));
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
		if (!m_graph_translation.populations.contains(descriptor)) {
			LOG4CXX_ERROR(
			    logger, "Network graph doesn't contain graph translation for population ("
			                << descriptor << ").");
			return false;
		}
		if (m_graph_translation.populations.at(descriptor).size() != pop.neurons.size()) {
			LOG4CXX_ERROR(
			    logger, "Network graph graph translation for population ("
			                << descriptor << ") doesn't contain translation for all neurons.");
			return false;
		}
		for (size_t n = 0; n < pop.neurons.size(); ++n) {
			for (auto const& [compartment, ans] :
			     pop.neurons.at(n).coordinate.get_placed_compartments()) {
				if (!m_graph_translation.populations.at(descriptor).at(n).contains(compartment)) {
					LOG4CXX_ERROR(
					    logger, "Network graph graph translation for population ("
					                << descriptor << "), neuron (" << n
					                << ") doesn't contain translation for compartment ("
					                << compartment << ").");
					return false;
				}
				if (m_graph_translation.populations.at(descriptor).at(n).at(compartment).size() !=
				    ans.size()) {
					LOG4CXX_ERROR(
					    logger, "Network graph graph translation for population ("
					                << descriptor << "), neuron (" << n
					                << ") doesn't contain all atomic neuron's translations for "
					                   "compartment ("
					                << compartment << ").");
					return false;
				}
				for (size_t an = 0; an < ans.size(); ++an) {
					auto const& [vertex_descriptor, index_on_vertex] =
					    m_graph_translation.populations.at(descriptor).at(n).at(compartment).at(an);
					auto const& vertex = std::get<signal_flow::vertex::NeuronView>(
					    m_graph.get_vertex_property(vertex_descriptor));
					auto const& configs = vertex.get_configs();
					auto const& expected_label = m_graph_translation.spike_labels.at(descriptor)
					                                 .at(n)
					                                 .at(compartment)
					                                 .at(an);
					if (static_cast<bool>(expected_label) !=
					    static_cast<bool>(configs.at(index_on_vertex).label)) {
						LOG4CXX_ERROR(
						    logger,
						    "Abstract network on-chip neuron("
						        << std::boolalpha << descriptor << ", " << n << ", " << compartment
						        << ", " << an << ") event label ("
						        << static_cast<bool>(expected_label)
						        << ") does not match event output status (enabled/disabled) in "
						           "hardware "
						           "network NeuronView's config("
						        << static_cast<bool>(configs.at(index_on_vertex).label) << ").");
						return false;
					}
					if (expected_label) {
						SpikeLabel actual_label;
						actual_label.set_neuron_backend_address_out(
						    *(configs.at(index_on_vertex).label));
						actual_label.set_neuron_event_output(
						    ans.at(an).toNeuronColumnOnDLS().toNeuronEventOutputOnDLS());
						actual_label.set_spl1_address(
						    SPL1Address(ans.at(an)
						                    .toNeuronColumnOnDLS()
						                    .toNeuronEventOutputOnDLS()
						                    .toNeuronEventOutputOnNeuronBackendBlock()));
						if (actual_label != *expected_label) {
							LOG4CXX_ERROR(
							    logger, "Abstract network on-chip neuron("
							                << descriptor << ", " << n << ", " << compartment
							                << ", " << an << ") event label (" << *expected_label
							                << ") does not match event label in hardware "
							                   "network NeuronView's config("
							                << actual_label << ").");
							return false;
						}
					}
				}
			}
		}
	}

	// check that all on-chip background spike sources are represented
	for (auto const& [descriptor, population] : m_network->populations) {
		if (!std::holds_alternative<BackgroundSourcePopulation>(population)) {
			continue;
		}
		auto const& pop = std::get<BackgroundSourcePopulation>(population);
		if (!m_graph_translation.background_spike_source_vertices.contains(descriptor)) {
			LOG4CXX_ERROR(
			    logger, "No hardware network vertices for on-chip background spike source("
			                << descriptor << ").");
			return false;
		}
		for (auto const& [hemisphere, padi_bus] : pop.coordinate) {
			if (!m_graph_translation.background_spike_source_vertices.at(descriptor)
			         .contains(hemisphere)) {
				LOG4CXX_ERROR(
				    logger, "No hardware network vertex for on-chip background spike source("
				                << descriptor << ") on " << hemisphere << ".");
				return false;
			}
			auto const& vertex =
			    std::get<signal_flow::vertex::BackgroundSpikeSource>(m_graph.get_vertex_property(
			        m_graph_translation.background_spike_source_vertices.at(descriptor)
			            .at(hemisphere)));
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
		if (!std::holds_alternative<BackgroundSourcePopulation>(population)) {
			continue;
		}
		auto const& pop = std::get<BackgroundSourcePopulation>(population);
		size_t label_entry = 0;
		for (auto const& [hemisphere, padi_bus] : pop.coordinate) {
			auto const& vertex =
			    std::get<signal_flow::vertex::BackgroundSpikeSource>(m_graph.get_vertex_property(
			        m_graph_translation.background_spike_source_vertices.at(descriptor)
			            .at(hemisphere)));
			for (size_t i = 0; i < pop.size; ++i) {
				auto expected_label = m_graph_translation.spike_labels.at(descriptor)
				                          .at(i)
				                          .at(CompartmentOnLogicalNeuron())
				                          .at(label_entry);
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
		if (!m_graph_translation.projections.contains(descriptor)) {
			LOG4CXX_ERROR(
			    logger, "Network graph doesn't contain graph translation for projection ("
			                << descriptor << ").");
			return false;
		}
		if (m_graph_translation.projections.at(descriptor).size() !=
		    projection.connections.size()) {
			LOG4CXX_ERROR(
			    logger, "Network graph graph translation for projection ("
			                << descriptor << ") doesn't contain translation for all connections.");
			return false;
		}
		std::map<SynapseRowOnDLS, Receptor::Type> receptor_type_per_row;
		for (auto const& [hemisphere, vertex_descriptor] :
		     m_graph_translation.synapse_vertices.at(descriptor)) {
			// calculate synapse row receptor types
			for (auto const in_edge : boost::make_iterator_range(
			         boost::in_edges(vertex_descriptor, m_graph.get_graph()))) {
				auto const& synapse_driver = std::get<signal_flow::vertex::SynapseDriver>(
				    m_graph.get_vertex_property(boost::source(in_edge, m_graph.get_graph())));
				auto const& coordinate = synapse_driver.get_coordinate();
				auto const& config = synapse_driver.get_config();
				for (auto const row_on_synapse_driver : iter_all<SynapseRowOnSynapseDriver>()) {
					auto const row = coordinate.toSynapseRowOnSynram()[row_on_synapse_driver];
					switch (config.row_modes[row_on_synapse_driver]) {
						case SynapseDriverConfig::RowMode::excitatory: {
							receptor_type_per_row[SynapseRowOnDLS(
							    row, hemisphere.toSynramOnDLS())] = Receptor::Type::excitatory;
							break;
						}
						case SynapseDriverConfig::RowMode::inhibitory: {
							receptor_type_per_row[SynapseRowOnDLS(
							    row, hemisphere.toSynramOnDLS())] = Receptor::Type::inhibitory;
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
			auto const& synapse_array_view = std::get<signal_flow::vertex::SynapseArrayViewSparse>(
			    m_graph.get_vertex_property(vertex_descriptor));
			if (synapse_array_view.get_synram().toHemisphereOnDLS() != hemisphere) {
				LOG4CXX_ERROR(
				    logger, "SynapseArrayView vertex descriptor hemisphere key does not match "
				            "vertex property.");
				return false;
			}
		}
		for (size_t c = 0;
		     auto const& placed_connections : m_graph_translation.projections.at(descriptor)) {
			Projection::Connection::Weight hardware_weight(0);
			for (auto const& [vertex_descriptor, index_on_vertex] : placed_connections) {
				auto const& synapse_array_view =
				    std::get<signal_flow::vertex::SynapseArrayViewSparse>(
				        m_graph.get_vertex_property(vertex_descriptor));
				if (index_on_vertex >= synapse_array_view.get_synapses().size()) {
					LOG4CXX_ERROR(
					    logger, "NetworkGraph projection connection translation contains "
					            "out-of-bounds index on SynapseArrayViewSparse vertex.");
					return false;
				}
				auto const& synapse = synapse_array_view.get_synapses()[index_on_vertex];
				hardware_weight += Projection::Connection::Weight(synapse.weight.value());
				auto const& hardware_receptor_type = receptor_type_per_row.at(SynapseRowOnDLS(
				    synapse_array_view.get_rows()[synapse.index_row],
				    synapse_array_view.get_synram()));
				if (hardware_receptor_type != projection.receptor.type) {
					LOG4CXX_ERROR(
					    logger, "Abstract network receptor type does not match hardware network "
					            "receptor type.");
					return false;
				}
				// check post-synaptic neuron is part of target compartment
				AtomicNeuronOnDLS const neuron_post(
				    synapse_array_view.get_columns()[synapse.index_column].toNeuronColumnOnDLS(),
				    synapse_array_view.get_synram().toNeuronRowOnDLS());
				auto const& population_post =
				    std::get<Population>(m_network->populations.at(projection.population_post));
				auto const compartment_neurons_post =
				    population_post.neurons.at(projection.connections.at(c).index_post.first)
				        .coordinate.get_placed_compartments()
				        .at(projection.connections.at(c).index_post.second);
				if (std::find(
				        compartment_neurons_post.begin(), compartment_neurons_post.end(),
				        neuron_post) == compartment_neurons_post.end()) {
					LOG4CXX_ERROR(
					    logger, "Abstract network compartment ("
					                << hate::join_string(
					                       compartment_neurons_post.begin(),
					                       compartment_neurons_post.end(), ", ")
					                << ") doesn't contain target atomic neuroni (" << neuron_post
					                << ").");
					return false;
				}
				// check target atomic neuron features receptor type matching connection requirement
				size_t const neuron_on_compartment = std::distance(
				    compartment_neurons_post.begin(),
				    std::find(
				        compartment_neurons_post.begin(), compartment_neurons_post.end(),
				        neuron_post));
				auto const& receptors_post =
				    population_post.neurons.at(projection.connections.at(c).index_post.first)
				        .compartments.at(projection.connections.at(c).index_post.second)
				        .receptors.at(neuron_on_compartment);
				if (std::none_of(
				        receptors_post.begin(), receptors_post.end(),
				        [hardware_receptor_type](auto const& receptor) {
					        return receptor.type == hardware_receptor_type;
				        })) {
					LOG4CXX_ERROR(
					    logger, "Target neuron compartment atomic neuron doesn't feature requested "
					            "receptor type.");
					return false;
				}
			}
			if (hardware_weight != projection.connections.at(c).weight) {
				LOG4CXX_ERROR(
				    logger, "Abstract network synaptic weight does not match accumulated hardware "
				            "network synaptic weights.");
				return false;
			}
			c++;
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
			    logger, "Abstract network connectum missing in hardware network:\n"
			                << hate::indent(
			                       hate::join_string(
			                           missing_in_hardware_network.begin(),
			                           missing_in_hardware_network.end(), "\n"),
			                       "\t\t"));
			LOG4CXX_ERROR(
			    logger, "Hardware network connectum missing in abstract network:\n"
			                << hate::indent(
			                       hate::join_string(
			                           missing_in_abstract_network.begin(),
			                           missing_in_abstract_network.end(), "\n"),
			                       "\t\t"));
			LOG4CXX_ERROR(
			    logger, "Abstract network connectum (size: "
			                << connectum_from_abstract_network.size()
			                << ") does not match hardware network connectum (size: "
			                << connectum_from_hardware_network.size()
			                << "):\n\tabstract network:\n "
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
			for (auto const& [hemisphere, vertex_descriptor] :
			     m_graph_translation.synapse_vertices.at(descriptor)) {
				auto const& synapse_array_view =
				    std::get<signal_flow::vertex::SynapseArrayViewSparse>(
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

std::vector<NetworkGraph::PlacedConnection> NetworkGraph::get_placed_connection(
    ProjectionOnNetwork const descriptor, size_t const index) const
{
	std::vector<NetworkGraph::PlacedConnection> ret;

	auto const& graph_translation = m_graph_translation.projections.at(descriptor).at(index);

	for (auto const& [vertex_descriptor, index_on_vertex] : graph_translation) {
		auto const& synapse_array_view_sparse =
		    std::get<signal_flow::vertex::SynapseArrayViewSparse>(
		        m_graph.get_vertex_property(vertex_descriptor));
		auto const rows = synapse_array_view_sparse.get_rows();
		auto const columns = synapse_array_view_sparse.get_columns();
		auto const synapses = synapse_array_view_sparse.get_synapses();

		assert(index_on_vertex < synapses.size());
		auto const& synapse = synapses[index_on_vertex];
		assert(synapse.index_row < rows.size());
		assert(synapse.index_column < columns.size());
		PlacedConnection placed_connection{
		    synapse.weight,
		    halco::hicann_dls::vx::v3::SynapseRowOnDLS(
		        rows[synapse.index_row], synapse_array_view_sparse.get_synram()),
		    columns[synapse.index_column]};
		ret.push_back(placed_connection);
	}
	return ret;
}

NetworkGraph::PlacedConnections NetworkGraph::get_placed_connections(
    ProjectionOnNetwork const descriptor) const
{
	PlacedConnections placed_connections;
	assert(m_network);
	for (size_t i = 0; i < m_network->projections.at(descriptor).connections.size(); ++i) {
		placed_connections.push_back(get_placed_connection(descriptor, i));
	}
	return placed_connections;
}

std::ostream& operator<<(std::ostream& os, NetworkGraph::PlacedConnection const& placed_connection)
{
	os << "PlacedConnection(\n\t" << placed_connection.weight << "\n\t"
	   << placed_connection.synapse_row << "\n\t" << placed_connection.synapse_on_row << "\n)";
	return os;
}

} // namespace grenade::vx::network
