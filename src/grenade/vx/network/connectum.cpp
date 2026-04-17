#include "grenade/vx/network/connectum.h"

#include "grenade/common/inter_topology_hyper_edge/fixture.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/population.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/mapping/external_source_neuron.h"
#include "grenade/vx/network/abstract/mapping/locally_placed_neuron.h"
#include "grenade/vx/network/abstract/mapping/poisson_source_neuron.h"
#include "grenade/vx/network/abstract/mapping/uncalibrated_signed_synapse.h"
#include "grenade/vx/network/abstract/mapping/uncalibrated_synapse.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/network/abstract/population_cell/poisson_source.h"
#include "grenade/vx/network/build_connection_routing.h"
#include "grenade/vx/network/exception.h"
#include "grenade/vx/network/routing/greedy/routing_constraints.h"
#include "grenade/vx/network/vertex/transformation/spike_lookup.h"
#include "grenade/vx/signal_flow/vertex/background_spike_source.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view_sparse.h"
#include "grenade/vx/signal_flow/vertex/synapse_driver.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include "halco/hicann-dls/vx/v3/event.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/timer.h"
#include <functional>
#include <ostream>
#include <log4cxx/logger.h>

namespace grenade::vx::network {

bool ConnectumConnection::operator==(ConnectumConnection const& other) const
{
	return std::tie(source, target, receptor_type, projection, connection_on_projection) ==
	       std::tie(
	           other.source, other.target, other.receptor_type, other.projection,
	           other.connection_on_projection);
}

bool ConnectumConnection::operator!=(ConnectumConnection const& other) const
{
	return !(*this == other);
}

bool ConnectumConnection::operator<(ConnectumConnection const& other) const
{
	return std::tie(source, target, receptor_type, projection, connection_on_projection) <
	       std::tie(
	           other.source, other.target, other.receptor_type, other.projection,
	           other.connection_on_projection);
}

std::ostream& operator<<(std::ostream& os, ConnectumConnection const& config)
{
	return os << "ConnectumConnection(\n"
	          << "\tsource: (" << std::get<0>(config.source) << ", " << std::get<1>(config.source)
	          << ", " << std::get<2>(config.source) << ")\n\ttarget: " << config.target
	          << "\n\treceptor_type: " << config.receptor_type
	          << "\n\tprojection: " << config.projection
	          << "\n\tconnection: " << config.connection_on_projection << "\n)";
}


std::vector<ConnectumConnection> generate_connectum_from_abstract_network(
    grenade::common::LinkedTopology const& topology)
{
	std::vector<ConnectumConnection> connectum;

	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::vector<grenade::common::VertexOnTopology>>
	    partitioned_vertices_per_execution_instance;

	for (auto const& vertex_descriptor : topology.get_reference().vertices()) {
		auto const& partitioned_vertex = dynamic_cast<grenade::common::PartitionedVertex const&>(
		    topology.get_reference().get(vertex_descriptor));
		partitioned_vertices_per_execution_instance
		    [partitioned_vertex.get_execution_instance_on_executor().value()]
		        .push_back(vertex_descriptor);
	}

	for (auto const& [execution_instance_on_executor, partitioned_vertex_descriptors] :
	     partitioned_vertices_per_execution_instance) {
		auto const connection_routing =
		    build_connection_routing(topology, partitioned_vertex_descriptors);
		routing::greedy::RoutingConstraints constraints(
		    topology, partitioned_vertex_descriptors, connection_routing);

		for (auto const& connection : constraints.get_external_connections()) {
			connectum.push_back(ConnectumConnection{
			    connection.source_descriptor, connection.target, connection.receptor_type,
			    connection.descriptor.first, connection.descriptor.second});
		}

		for (auto const& connection : constraints.get_background_connections()) {
			connectum.push_back(ConnectumConnection{
			    connection.source_descriptor, connection.target, connection.receptor_type,
			    connection.descriptor.first, connection.descriptor.second});
		}

		for (auto const& connection : constraints.get_internal_connections()) {
			connectum.push_back(ConnectumConnection{
			    connection.source_descriptor, connection.target, connection.receptor_type,
			    connection.descriptor.first, connection.descriptor.second});
		}
	}
	return connectum;
}

namespace {

struct HardwareConnectionPath
{
	signal_flow::vertex::CrossbarNode const& crossbar_node;
	signal_flow::vertex::CrossbarNode::Parameterization const& crossbar_node_parameterization;
	signal_flow::vertex::SynapseDriver const& synapse_driver;
	signal_flow::vertex::SynapseDriver::Parameterization const& synapse_driver_parameterization;
	signal_flow::vertex::SynapseArrayViewSparse const& synapse_array_view;
	signal_flow::vertex::SynapseArrayViewSparse::Parameterization::Labels const&
	    synapse_array_view_parameterization;
	size_t matching_synapse;
	std::pair<grenade::common::VertexOnTopology, size_t> matching_projection_connection;
};


// generated with chat-gpt
std::vector<halco::hicann_dls::vx::v3::SpikeLabel> iterate_with_mask(uintmax_t base, uintmax_t mask)
{
	// collect positions of bits set in mask
	std::vector<uintmax_t> positions;
	for (uintmax_t i = 0; i < 32; i++) {
		if (mask & (1 << i))
			positions.push_back(i);
	}

	std::vector<halco::hicann_dls::vx::v3::SpikeLabel> results;
	uintmax_t combos = 1 << positions.size(); // 2^k combinations
	results.reserve(combos);

	for (uintmax_t combo = 0; combo < combos; combo++) {
		uintmax_t val = base;
		for (uintmax_t j = 0; j < positions.size(); j++) {
			uintmax_t pos = positions[j];
			if (combo & (1 << j))
				val |= (1 << pos); // set bit
			else
				val &= ~(1 << pos); // clear bit
		}
		results.push_back(halco::hicann_dls::vx::v3::SpikeLabel(val));
	}
	return results;
}


} // namespace

std::vector<ConnectumConnection> generate_connectum_from_hardware_network(
    grenade::common::LinkedTopology const& topology)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;

	auto logger = log4cxx::Logger::getLogger("grenade.generate_connectum_from_hardware_network()");
	hate::Timer timer;

	hate::Timer timer_split_population_descriptors;
	std::vector<grenade::common::VertexOnTopology> external_population_descriptors;
	std::vector<grenade::common::VertexOnTopology> internal_population_descriptors;
	std::vector<grenade::common::VertexOnTopology> background_population_descriptors;
	std::map<grenade::common::ExecutionInstanceOnExecutor, std::vector<size_t>>
	    external_populations_per_execution_instance;
	std::map<grenade::common::ExecutionInstanceOnExecutor, std::vector<size_t>>
	    internal_populations_per_execution_instance;
	std::map<grenade::common::ExecutionInstanceOnExecutor, std::vector<size_t>>
	    background_populations_per_execution_instance;
	for (auto const& descriptor : topology.get_reference().vertices()) {
		if (auto const population = dynamic_cast<grenade::common::Population const*>(
		        &topology.get_reference().get(descriptor));
		    population) {
			if (!population->get_time_domain()) {
				continue;
			}
			if (auto const external_source =
			        dynamic_cast<abstract::ExternalSourceNeuron const*>(&population->get_cell());
			    external_source) {
				external_populations_per_execution_instance
				    [population->get_execution_instance_on_executor().value()]
				        .push_back(external_population_descriptors.size());
				external_population_descriptors.push_back(
				    descriptor); // TODO(SpikeIO): add case for SpikeIOInputNeuron
			} else if (auto const poisson_source =
			               dynamic_cast<abstract::PoissonSourceNeuron const*>(
			                   &population->get_cell());
			           poisson_source) {
				background_populations_per_execution_instance
				    [population->get_execution_instance_on_executor().value()]
				        .push_back(background_population_descriptors.size());
				background_population_descriptors.push_back(descriptor);
			} else if (auto const internal_source =
			               dynamic_cast<abstract::LocallyPlacedNeuron const*>(
			                   &population->get_cell());
			           internal_source) {
				internal_populations_per_execution_instance
				    [population->get_execution_instance_on_executor().value()]
				        .push_back(internal_population_descriptors.size());
				internal_population_descriptors.push_back(descriptor);
			} else {
				throw std::logic_error("Population type not supported.");
			}
		}
	}
	LOG4CXX_TRACE(
	    logger, "Splitted population descriptors by population type in "
	                << timer_split_population_descriptors.print() << ".");

	hate::Timer timer_build_labels;
	std::vector<std::reference_wrapper<
	    std::map<size_t, std::vector<halco::hicann_dls::vx::v3::SpikeLabel>> const>>
	    external_population_labels;
	for (auto const& external_population_descriptor : external_population_descriptors) {
		for (auto const& mapping_descriptor :
		     topology.inter_graph_hyper_edges_by_reference(external_population_descriptor)) {
			auto const& external_mapping =
			    dynamic_cast<abstract::ExternalSourceNeuronMapping const&>(
			        topology.get(mapping_descriptor));
			auto const& references = topology.references(mapping_descriptor);
			size_t const reference_vertex_index = std::distance(
			    references.begin(),
			    std::find(references.begin(), references.end(), external_population_descriptor));
			external_population_labels.push_back(
			    external_mapping.labels.at(reference_vertex_index));
		}
	}
	std::vector<std::map<
	    size_t, std::map<
	                halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
	                halco::hicann_dls::vx::v3::SpikeLabel>>>
	    internal_population_labels(internal_population_descriptors.size());
	for (size_t pop_index = 0;
	     auto const& internal_population_descriptor : internal_population_descriptors) {
		for (auto const& mapping_descriptor :
		     topology.inter_graph_hyper_edges_by_reference(internal_population_descriptor)) {
			if (auto const mapping = dynamic_cast<abstract::LocallyPlacedNeuronMapping const*>(
			        &topology.get(mapping_descriptor));
			    mapping) {
				auto const& neuron = dynamic_cast<abstract::LocallyPlacedNeuron const&>(
				    dynamic_cast<grenade::common::Population const&>(
				        topology.get_reference().get(internal_population_descriptor))
				        .get_cell());
				auto const& links = topology.links(mapping_descriptor);
				std::map<
				    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS,
				    halco::hicann_dls::vx::v3::SpikeLabel>
				    spike_labels;
				for (size_t mapped_vertex_index = 0; auto const& local_labels : mapping->labels) {
					auto const& neuron_view = dynamic_cast<signal_flow::vertex::NeuronView const&>(
					    topology.get(links.at(mapped_vertex_index)));
					for (size_t i = 0; auto const& column : neuron_view.get_columns()) {
						if (local_labels.at(i)) {
							halco::hicann_dls::vx::v3::SpikeLabel local_label;
							local_label.set_neuron_backend_address_out(*local_labels.at(i));
							halco::hicann_dls::vx::v3::AtomicNeuronOnDLS atomic_neuron(
							    column, neuron_view.row);
							local_label.set_neuron_event_output(column.toNeuronEventOutputOnDLS());
							local_label.set_spl1_address(halco::hicann_dls::vx::v3::SPL1Address(
							    column.toNeuronEventOutputOnDLS()
							        .toNeuronEventOutputOnNeuronBackendBlock()));
							spike_labels[atomic_neuron] = local_label;
						}
						i++;
					}
					mapped_vertex_index++;
				}
				for (auto const& [index_pre, anchor] : mapping->anchors) {
					halco::hicann_dls::vx::v3::LogicalNeuronOnDLS logical_neuron(
					    neuron.shape, anchor.second);
					for (auto const& [compartment_on_neuron, atomic_neurons] :
					     logical_neuron.get_placed_compartments()) {
						if (neuron.compartments
						        .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
						        .spike_master) {
							auto const& atomic_neuron = atomic_neurons.at(
							    neuron.compartments
							        .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
							        .spike_master->atomic_neuron_on_compartment);
							if (!spike_labels.contains(atomic_neuron)) {
								// no spike label
								// -> disabled
								// spiking
								// mechanism
								continue;
							}
							auto const& local_label = spike_labels.at(atomic_neuron);
							internal_population_labels.at(
							    pop_index)[index_pre][compartment_on_neuron] = local_label;
						}
					}
				}
			}
		}
		pop_index++;
	}
	std::vector<std::map<
	    halco::hicann_dls::vx::v3::BackgroundSpikeSourceOnDLS,
	    std::vector<halco::hicann_dls::vx::v3::SpikeLabel>>>
	    background_population_labels;
	for (auto const& background_population_descriptor : background_population_descriptors) {
		background_population_labels.push_back({});
		for (auto const& mapping_descriptor :
		     topology.inter_graph_hyper_edges_by_reference(background_population_descriptor)) {
			auto const& mapping = dynamic_cast<abstract::PoissonSourceNeuronMapping const&>(
			    topology.get(mapping_descriptor));
			auto const& links = topology.links(mapping_descriptor);
			for (size_t link_vertex_index = 0; link_vertex_index < links.size();
			     ++link_vertex_index) {
				auto const& background_spike_source =
				    dynamic_cast<signal_flow::vertex::BackgroundSpikeSource const&>(
				        topology.get(links.at(link_vertex_index)));
				auto const& label = mapping.labels.at(link_vertex_index);
				auto const& mask = mapping.masks.at(link_vertex_index);
				background_population_labels.back()[background_spike_source.coordinate] =
				    iterate_with_mask(label, mask);
			}
		}
	}
	LOG4CXX_TRACE(
	    logger, "Collected local labels of sources in " << timer_build_labels.print() << ".");

	std::vector<ConnectumConnection> connectum;
	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::vector<grenade::common::VertexOnTopology>>
	    mapped_vertices_per_execution_instance;

	for (auto const& vertex_descriptor : topology.vertices()) {
		auto const& partitioned_vertex = dynamic_cast<grenade::common::PartitionedVertex const&>(
		    topology.get(vertex_descriptor));
		if (partitioned_vertex.get_time_domain()) {
			mapped_vertices_per_execution_instance
			    [partitioned_vertex.get_execution_instance_on_executor().value()]
			        .push_back(vertex_descriptor);
		}
	}

	for (auto const& [id, mapped_vertices] : mapped_vertices_per_execution_instance) {
		hate::Timer timer_extract_vertex_properties;
		std::vector<std::reference_wrapper<signal_flow::vertex::CrossbarNode const>> crossbar_nodes;
		std::vector<
		    std::reference_wrapper<signal_flow::vertex::CrossbarNode::Parameterization const>>
		    crossbar_node_parameterizations;
		std::vector<std::reference_wrapper<signal_flow::vertex::SynapseDriver const>>
		    synapse_drivers;
		std::vector<
		    std::reference_wrapper<signal_flow::vertex::SynapseDriver::Parameterization const>>
		    synapse_driver_parameterizations;
		std::vector<std::pair<
		    std::reference_wrapper<signal_flow::vertex::SynapseArrayViewSparse const>,
		    std::pair<grenade::common::VertexOnTopology, std::vector<size_t>>>>
		    synapse_array_views;
		std::vector<signal_flow::vertex::SynapseArrayViewSparse::Parameterization::Labels>
		    synapse_array_view_parameterizations;
		std::vector<std::reference_wrapper<signal_flow::vertex::BackgroundSpikeSource const>>
		    background_spike_sources;
		std::set<NeuronEventOutputOnDLS> active_neuron_event_outputs;

		std::map<grenade::common::VertexOnTopology, grenade::common::VertexOnTopology>
		    projection_descriptor_per_synapse_vertex;
		for (auto const& mapped_vertex : mapped_vertices) {
			if (auto const synapse_array_view_sparse =
			        dynamic_cast<signal_flow::vertex::SynapseArrayViewSparse const*>(
			            &topology.get(mapped_vertex));
			    synapse_array_view_sparse) {
				for (auto const& inter_graph_hyper_edge_descriptor :
				     topology.inter_graph_hyper_edges_by_linked(mapped_vertex)) {
					auto const& partitioned_vertex_descriptor =
					    topology.references(inter_graph_hyper_edge_descriptor).at(0);
					projection_descriptor_per_synapse_vertex[mapped_vertex] =
					    partitioned_vertex_descriptor;
				}
			}
		}

		for (auto const mapped_vertex : mapped_vertices) {
			if (auto const crossbar_node = dynamic_cast<signal_flow::vertex::CrossbarNode const*>(
			        &topology.get(mapped_vertex));
			    crossbar_node) {
				if (std::find_if(
				        crossbar_nodes.begin(), crossbar_nodes.end(),
				        [&crossbar_node](auto const& a) { return a.get() == *crossbar_node; }) ==
				    crossbar_nodes.end()) {
					crossbar_nodes.push_back(*crossbar_node);
					crossbar_node_parameterizations.push_back(
					    dynamic_cast<signal_flow::vertex::CrossbarNode::Parameterization const&>(
					        dynamic_cast<grenade::common::FixtureInterTopologyHyperEdge const&>(
					            topology.get(
					                *topology.inter_graph_hyper_edges_by_linked(mapped_vertex)
					                     .begin()))
					            .port_data_per_port.get(1)));
				}
			} else if (auto const synapse_driver =
			               dynamic_cast<signal_flow::vertex::SynapseDriver const*>(
			                   &topology.get(mapped_vertex));
			           synapse_driver) {
				if (std::find_if(
				        synapse_drivers.begin(), synapse_drivers.end(),
				        [&synapse_driver](auto const& a) { return a.get() == *synapse_driver; }) ==
				    synapse_drivers.end()) {
					synapse_drivers.push_back(*synapse_driver);
					synapse_driver_parameterizations.push_back(
					    dynamic_cast<signal_flow::vertex::SynapseDriver::Parameterization const&>(
					        dynamic_cast<grenade::common::FixtureInterTopologyHyperEdge const&>(
					            topology.get(
					                *topology.inter_graph_hyper_edges_by_linked(mapped_vertex)
					                     .begin()))
					            .port_data_per_port.get(1)));
				}
			} else if (auto const synapse_array_view =
			               dynamic_cast<signal_flow::vertex::SynapseArrayViewSparse const*>(
			                   &topology.get(mapped_vertex));
			           synapse_array_view) {
				if (std::find_if(
				        synapse_array_views.begin(), synapse_array_views.end(),
				        [&synapse_array_view](auto const& a) {
					        return a.first.get() == *synapse_array_view;
				        }) == synapse_array_views.end()) {
					auto const& projection_descriptor =
					    projection_descriptor_per_synapse_vertex.at(mapped_vertex);
					auto const& inter_topology_hyper_edge_descriptor =
					    *topology.inter_graph_hyper_edges_by_reference(projection_descriptor)
					         .begin();
					auto const& links = topology.links(inter_topology_hyper_edge_descriptor);
					size_t const mapped_vertex_index = std::distance(
					    links.begin(), std::find(links.begin(), links.end(), mapped_vertex));
					abstract::UncalibratedSynapseMapping::Translation projection_connections;
					auto const& mapping = dynamic_cast<abstract::UncalibratedSynapseMapping const&>(
					    topology.get(inter_topology_hyper_edge_descriptor));
					projection_connections = mapping.translation;
					synapse_array_view_parameterizations.push_back(
					    mapping.labels.at(mapped_vertex_index));

					std::vector<std::pair<size_t, size_t>>
					    connection_on_projection_per_synapse_on_vertex_map;
					for (auto const& [c, synapses_per_connection] : projection_connections) {
						auto const& synapses_per_connection_per_vertex =
						    synapses_per_connection.at(mapped_vertex_index);
						for (auto const& synapse_on_vertex : synapses_per_connection_per_vertex) {
							connection_on_projection_per_synapse_on_vertex_map.push_back(
							    std::make_pair(synapse_on_vertex, c));
						}
					}
					std::sort(
					    connection_on_projection_per_synapse_on_vertex_map.begin(),
					    connection_on_projection_per_synapse_on_vertex_map.end(),
					    [](auto const& a, auto const& b) { return a.first < b.first; });
					assert(
					    connection_on_projection_per_synapse_on_vertex_map.size() ==
					    synapse_array_view->get_synapses().size());
					std::vector<size_t> connection_on_projection_per_synapse_on_vertex;
					for (auto const& [_, connection_on_projection] :
					     connection_on_projection_per_synapse_on_vertex_map) {
						connection_on_projection_per_synapse_on_vertex.push_back(
						    connection_on_projection);
					}
					synapse_array_views.emplace_back(
					    *synapse_array_view,
					    std::make_pair(
					        projection_descriptor, connection_on_projection_per_synapse_on_vertex));
				}
			} else if (auto const background_spike_source =
			               dynamic_cast<signal_flow::vertex::BackgroundSpikeSource const*>(
			                   &topology.get(mapped_vertex));
			           background_spike_source) {
				if (std::find_if(
				        background_spike_sources.begin(), background_spike_sources.end(),
				        [&background_spike_source](auto const& a) {
					        return a.get() == *background_spike_source;
				        }) == background_spike_sources.end()) {
					background_spike_sources.push_back(*background_spike_source);
				}
			} else if (auto const neuron_view =
			               dynamic_cast<signal_flow::vertex::NeuronView const*>(
			                   &topology.get(mapped_vertex));
			           neuron_view) {
				for (auto const& column : neuron_view->get_columns()) {
					active_neuron_event_outputs.insert(column.toNeuronEventOutputOnDLS());
				}
			}
		}
		LOG4CXX_TRACE(
		    logger, "Extracted unique vertex properties in "
		                << timer_extract_vertex_properties.print() << ".");

		hate::Timer timer_check_non_overlapping_vertices;
		// check that no vertices describe overlapping locations on the hardware
		{
			std::set<signal_flow::vertex::CrossbarNode::Coordinate> unique;
			for (auto const& crossbar_node : crossbar_nodes) {
				unique.insert(crossbar_node.get().coordinate);
			}
			if (crossbar_nodes.size() != unique.size()) {
				throw InvalidNetworkGraph("CrossbarNode vertices overlap on hardware.");
			}
		}
		{
			std::set<signal_flow::vertex::SynapseDriver::Coordinate> unique;
			for (auto const& synapse_driver : synapse_drivers) {
				unique.insert(synapse_driver.get().coordinate);
			}
			if (synapse_drivers.size() != unique.size()) {
				throw InvalidNetworkGraph("SynapseDriver vertices overlap on hardware.");
			}
		}
		{
			std::set<signal_flow::vertex::BackgroundSpikeSource::Coordinate> unique;
			for (auto const& background_spike_source : background_spike_sources) {
				unique.insert(background_spike_source.get().coordinate);
			}
			if (background_spike_sources.size() != unique.size()) {
				throw InvalidNetworkGraph("BackgroundSpikeSource vertices overlap on hardware.");
			}
		}
		{
			std::set<std::pair<SynapseRowOnDLS, SynapseOnSynapseRow>> unique;
			size_t num_synapses = 0;
			for (auto const& [synapse_array_view, _] : synapse_array_views) {
				for (auto const& synapse : synapse_array_view.get().get_synapses()) {
					unique.insert(std::pair{
					    SynapseRowOnDLS(
					        synapse_array_view.get().get_rows()[synapse.index_row],
					        synapse_array_view.get().get_synram()),
					    synapse_array_view.get().get_columns()[synapse.index_column]});
					num_synapses++;
				}
			}
			if (num_synapses != unique.size()) {
				throw InvalidNetworkGraph("Synapse vertices overlap on hardware.");
			}
		}
		LOG4CXX_TRACE(
		    logger, "Checked for non-overlapping vertices in "
		                << timer_check_non_overlapping_vertices.print() << ".");

		// get connectum of a single path given an event label
		auto const process_path =
		    [&connectum](
		        HardwareConnectionPath const& path,
		        std::tuple<
		            grenade::common::VertexOnTopology, size_t,
		            halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron> const& descriptor,
		        halco::hicann_dls::vx::v3::SpikeLabel const& label) {
			    auto const neuron_label = label.get_neuron_label();
			    auto const target = path.crossbar_node_parameterization.config.get_target();
			    auto const mask = path.crossbar_node_parameterization.config.get_mask();
			    if ((neuron_label & mask) != target) { // crossbar node drops event
				    return;
			    }
			    auto const syndrv_mask =
			        path.synapse_driver_parameterization.row_address_compare_mask;
			    if ((path.synapse_driver.coordinate.toSynapseDriverOnSynapseDriverBlock()
			             .toSynapseDriverOnPADIBus() &
			         syndrv_mask) !=
			        (label.get_row_select_address() & syndrv_mask)) { // synapse driver drops event
				    return;
			    }
			    auto const synapse_label = label.get_synapse_label();
			    auto const synram = path.synapse_array_view.get_synram();
			    auto const& i_syn = path.matching_synapse;
			    auto const& [projection, connection_on_projection] =
			        path.matching_projection_connection;
			    auto const& synapse = path.synapse_array_view.get_synapses()[i_syn];
			    auto const& synapse_parameterization_label =
			        path.synapse_array_view_parameterization.at(i_syn);
			    if (synapse_parameterization_label != synapse_label) {
				    return;
			    }
			    auto const& row = path.synapse_array_view.get_rows()[synapse.index_row];
			    auto const& column = path.synapse_array_view.get_columns()[synapse.index_column];
			    AtomicNeuronOnDLS neuron(column.toNeuronColumnOnDLS(), synram.toNeuronRowOnDLS());
			    auto const row_on_synapse_driver = row.toSynapseRowOnSynapseDriver();
			    switch (path.synapse_driver_parameterization.row_modes[row_on_synapse_driver]) {
				    case SynapseDriverConfig::RowMode::excitatory: {
					    connectum.push_back(ConnectumConnection{
					        descriptor, neuron, Receptor::Type::excitatory, projection,
					        connection_on_projection});
					    break;
				    }
				    case SynapseDriverConfig::RowMode::inhibitory: {
					    connectum.push_back(ConnectumConnection{
					        descriptor, neuron, Receptor::Type::inhibitory, projection,
					        connection_on_projection});
					    break;
				    }
				    case SynapseDriverConfig::RowMode::excitatory_and_inhibitory: {
					    throw std::runtime_error("Synapse in hardware network features "
					                             "excitatory_and_inhibitory row mode.");
				    }
				    case SynapseDriverConfig::RowMode::disabled: {
					    std::stringstream ss;
					    ss << "Synapse in hardware network features disabled row mode at "
					       << SynapseRowOnDLS(row, synram) << path.synapse_driver << ".";
					    throw std::runtime_error(ss.str());
				    }
				    default: {
					    throw std::logic_error("Synapse driver row mode not supported.");
				    }
			    }
		    };

		hate::Timer timer_extract_hardware_connection_paths;
		std::vector<HardwareConnectionPath> hardware_connection_paths;
		for (size_t cb = 0; auto const crossbar_node : crossbar_nodes) {
			if (!crossbar_node.get()
			         .coordinate.toCrossbarOutputOnDLS()
			         .toPADIBusOnDLS()) { // not forwarding to PADI-bus
				cb++;
				continue;
			}
			for (size_t sd = 0; auto const synapse_driver : synapse_drivers) {
				if (*(crossbar_node.get().coordinate.toCrossbarOutputOnDLS().toPADIBusOnDLS()) !=
				    PADIBusOnDLS(
				        synapse_driver.get()
				            .coordinate.toSynapseDriverOnSynapseDriverBlock()
				            .toPADIBusOnPADIBusBlock(),
				        synapse_driver.get()
				            .coordinate.toSynapseDriverBlockOnDLS()
				            .toPADIBusBlockOnDLS())) { // PADI-bus of synapse driver and
					                                   // crossbar node don't match
					sd++;
					continue;
				}
				auto const synapse_rows = synapse_driver.get().coordinate.toSynapseRowOnSynram();
				for (size_t sa = 0; auto const& synapse_array_view : synapse_array_views) {
					if (synapse_driver.get()
					        .coordinate.toSynapseDriverBlockOnDLS()
					        .toSynramOnDLS() !=
					    synapse_array_view.first.get()
					        .get_synram()) { // hemisphere of synapse driver and
						// synapse array view does not match
						sa++;
						continue;
					}
					if (std::none_of(
					        synapse_rows.begin(), synapse_rows.end(),
					        [&synapse_array_view](auto const& row) {
						        auto const it = std::find(
						            synapse_array_view.first.get().get_rows().begin(),
						            synapse_array_view.first.get().get_rows().end(), row);
						        return it != synapse_array_view.first.get().get_rows().end();
					        })) {
						sa++;
						continue;
					}
					for (auto const row_on_synapse_driver : iter_all<SynapseRowOnSynapseDriver>()) {
						auto const& row = synapse_rows[row_on_synapse_driver];
						size_t i_syn = 0;
						for (auto const& synapse : synapse_array_view.first.get().get_synapses()) {
							if (synapse_array_view.first.get().get_rows()[synapse.index_row] ==
							    row) {
								hardware_connection_paths.push_back(HardwareConnectionPath{
								    crossbar_node.get(),
								    crossbar_node_parameterizations.at(cb).get(),
								    synapse_driver.get(),
								    synapse_driver_parameterizations.at(sd).get(),
								    synapse_array_view.first.get(),
								    synapse_array_view_parameterizations.at(sa), i_syn,
								    std::make_pair(
								        synapse_array_view.second.first,
								        synapse_array_view.second.second.at(i_syn))});
							}
							i_syn++;
						}
					}
					sa++;
				}
				sd++;
			}
			cb++;
		}
		LOG4CXX_TRACE(
		    logger, "Extracted hardware connection paths ("
		                << hardware_connection_paths.size() << ") in "
		                << timer_extract_hardware_connection_paths.print() << ".");
		LOG4CXX_TRACE(
		    logger, "Populations (" << external_population_descriptors.size() << "), ("
		                            << background_population_descriptors.size() << "), ("
		                            << internal_population_descriptors.size() << ")");

		// assumes event labels match between abstract network and hardware network
		hate::Timer timer_build_connectum;
		for (auto const& hardware_connection_path : hardware_connection_paths) {
			auto const spl1_address =
			    hardware_connection_path.crossbar_node.coordinate.toCrossbarInputOnDLS()
			        .toSPL1Address();
			auto const neuron_event_output =
			    hardware_connection_path.crossbar_node.coordinate.toCrossbarInputOnDLS()
			        .toNeuronEventOutputOnDLS();
			if (spl1_address) { // crossbar node from L2
				for (auto const& pop_index : external_populations_per_execution_instance[id]) {
					for (auto const& [index_pre, local_labels] :
					     external_population_labels.at(pop_index).get()) {
						for (auto const& spike_label : local_labels) {
							if (*spl1_address !=
							    spike_label.get_spl1_address()) { // wrong spl1 channel
								continue;
							}
							process_path(
							    hardware_connection_path,
							    std::tuple{
							        external_population_descriptors.at(pop_index), index_pre,
							        CompartmentOnLogicalNeuron()},
							    spike_label);
						}
					}
				}
			} else if (neuron_event_output) { // crossbar node from on-chip neuron
				if (!active_neuron_event_outputs.contains(*neuron_event_output)) {
					continue;
				}
				for (auto const& pop_index : internal_populations_per_execution_instance[id]) {
					for (auto const& [index_pre, labels_on_neuron] :
					     internal_population_labels.at(pop_index)) {
						for (auto const& [compartment_on_neuron, local_label] : labels_on_neuron) {
							if (*neuron_event_output !=
							    local_label.get_neuron_event_output()) { // wrong neuron
								                                         // event output
								continue;
							}
							process_path(
							    hardware_connection_path,
							    std::tuple{
							        internal_population_descriptors.at(pop_index), index_pre,
							        compartment_on_neuron},
							    local_label);
						}
					}
				}
			} else { // crossbar node from background spike source
				for (auto const& background_spike_source : background_spike_sources) {
					if (background_spike_source.get().coordinate.toCrossbarInputOnDLS() !=
					    hardware_connection_path.crossbar_node.coordinate
					        .toCrossbarInputOnDLS()) { // background source and crossbar
						                               // node don't match
						continue;
					}
					for (auto const& pop_index :
					     background_populations_per_execution_instance[id]) {
						for (size_t index_pre = 0; auto const& local_label :
						                           background_population_labels.at(pop_index).at(
						                               background_spike_source.get().coordinate)) {
							process_path(
							    hardware_connection_path,
							    std::tuple{
							        background_population_descriptors.at(pop_index), index_pre,
							        CompartmentOnLogicalNeuron()},
							    local_label);
							index_pre++;
						}
					}
				}
			}
		}
		LOG4CXX_TRACE(
		    logger, "Built connectum from hardware connection paths in "
		                << timer_build_connectum.print() << ".");
	}

	LOG4CXX_DEBUG(
	    logger,
	    "Generated connectum from hardware network in total duration of " << timer.print() << ".");

	return connectum;
}

} // namespace grenade::vx::network
