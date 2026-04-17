#include "grenade/vx/network/routing/greedy/routing_builder.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/population.h"
#include "grenade/common/projection.h"
#include "grenade/common/recorder.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/population_cell/delay.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/network/abstract/population_cell/poisson_source.h"
#include "grenade/vx/network/abstract/recorder/spike.h"
#include "grenade/vx/network/build_connection_weight_split.h"
#include "grenade/vx/network/exception.h"
#include "grenade/vx/network/routing/greedy/routing_constraints.h"
#include "grenade/vx/network/routing/greedy/synapse_driver_on_dls_manager.h"
#include "grenade/vx/signal_flow/vertex/background_spike_source.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "halco/hicann-dls/vx/v3/event.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/padi.h"
#include "halco/hicann-dls/vx/v3/routing_crossbar.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "halco/hicann-dls/vx/v3/synapse_driver.h"
#include "hate/algorithm.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include <map>
#include <set>
#include <unordered_set>
#include <boost/range/adaptor/reversed.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx::network::routing::greedy {

using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

RoutingBuilder::RoutingBuilder() : m_logger(log4cxx::Logger::getLogger("grenade.RoutingBuilder")) {}

void RoutingBuilder::route_internal_crossbar(
    SourceOnPADIBusManager::DisabledInternalRoutes& disabled_internal_routes,
    RoutingConstraints const& constraints,
    typed_array<RoutingConstraints::PADIBusConstraints, PADIBusOnDLS>& padi_bus_constraints,
    Result& result) const
{
	auto const& neuron_event_outputs_per_padi_bus =
	    constraints.get_neuron_event_outputs_on_padi_bus();

	// Disable crossbar node(s) to PADI-bus(ses), where no source neurons from the event output are
	// required.
	// Remove neuron sources which are then not present anymore on the PADI-bus.
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		std::set<AtomicNeuronOnDLS> neurons_on_padi_bus;
		std::set<AtomicNeuronOnDLS> only_recorded_neurons_on_padi_bus;
		for (auto const neuron_backend_block : iter_all<NeuronBackendConfigBlockOnDLS>()) {
			NeuronEventOutputOnDLS neuron_event_output(
			    NeuronEventOutputOnNeuronBackendBlock(padi_bus.value()), neuron_backend_block);
			if (!neuron_event_outputs_per_padi_bus.contains(padi_bus) ||
			    !neuron_event_outputs_per_padi_bus.at(padi_bus).contains(neuron_event_output)) {
				result.crossbar_nodes[CrossbarNodeOnDLS(
				    neuron_event_output.toCrossbarInputOnDLS(), padi_bus.toCrossbarOutputOnDLS())] =
				    haldls::vx::v3::CrossbarNode::drop_all;
				disabled_internal_routes[neuron_event_output].insert(
				    padi_bus.toPADIBusBlockOnDLS().toHemisphereOnDLS());
				LOG4CXX_DEBUG(
				    m_logger, "route_internal_crossbar(): Disabled crossbar node for "
				                  << neuron_event_output << " onto " << padi_bus << ".");
			} else {
				for (auto const& neuron : padi_bus_constraints[padi_bus].neuron_sources) {
					if (neuron.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS() ==
					    neuron_event_output) {
						neurons_on_padi_bus.insert(neuron);
					}
				}
				for (auto const& neuron : padi_bus_constraints[padi_bus].only_recorded_neurons) {
					if (neuron.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS() ==
					    neuron_event_output) {
						only_recorded_neurons_on_padi_bus.insert(neuron);
					}
				}
			}
		}
		padi_bus_constraints[padi_bus].neuron_sources = neurons_on_padi_bus;
		padi_bus_constraints[padi_bus].only_recorded_neurons = only_recorded_neurons_on_padi_bus;
	}
}

std::pair<
    std::vector<SourceOnPADIBusManager::InternalSource>,
    std::vector<std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>>>
RoutingBuilder::get_internal_sources(
    RoutingConstraints const& constraints,
    halco::common::typed_array<
        RoutingConstraints::PADIBusConstraints,
        halco::hicann_dls::vx::v3::PADIBusOnDLS> const& padi_bus_constraints) const
{
	// All neurons which are present at the PADI-bus are to be added.
	std::vector<SourceOnPADIBusManager::InternalSource> internal_sources;
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		auto const& local_padi_bus_constraints = padi_bus_constraints[padi_bus];
		for (auto const& neuron : local_padi_bus_constraints.neuron_sources) {
			SourceOnPADIBusManager::InternalSource source;
			source.neuron = neuron;
			if (std::find_if(
			        internal_sources.begin(), internal_sources.end(), [&neuron](auto const& s) {
				        return s.neuron == neuron;
			        }) == internal_sources.end()) {
				internal_sources.push_back(source);
			}
		}
		for (auto const& neuron : local_padi_bus_constraints.only_recorded_neurons) {
			SourceOnPADIBusManager::InternalSource source;
			source.neuron = neuron;
			if (std::find_if(
			        internal_sources.begin(), internal_sources.end(), [&neuron](auto const& s) {
				        return s.neuron == neuron;
			        }) == internal_sources.end()) {
				internal_sources.push_back(source);
			}
		}
		// calculate out-degree per target
		for (auto const& connection : local_padi_bus_constraints.internal_connections) {
			auto source_it = std::find_if(
			    internal_sources.begin(), internal_sources.end(),
			    [&connection](auto const& a) { return a.neuron == connection.source; });
			assert(source_it != internal_sources.end());
			auto& source = *source_it;
			if (!source.out_degree.contains(connection.receptor_type)) {
				source.out_degree[connection.receptor_type].fill(0);
			}
			source.out_degree.at(connection.receptor_type)[connection.target]++;
		}
	}
	// find descriptors
	// FIXME: only works for neurons which are sources to connections
	std::vector<std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>>
	    descriptors(internal_sources.size());
	auto const internal_source_neurons = constraints.get_internal_sources();
	for (size_t i = 0; auto& internal_source : internal_sources) {
		auto source_it = std::find_if(
		    internal_source_neurons.begin(), internal_source_neurons.end(),
		    [internal_source](auto const& a) { return a.second == internal_source.neuron; });
		assert(source_it != internal_source_neurons.end());
		descriptors.at(i) = source_it->first;
		i++;
	}
	LOG4CXX_DEBUG(
	    m_logger, "get_internal_sources(): Got " << internal_sources.size() << " sources.");
	return std::pair{internal_sources, descriptors};
}

std::pair<
    std::vector<SourceOnPADIBusManager::BackgroundSource>,
    std::vector<std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>>>
RoutingBuilder::get_background_sources(
    RoutingConstraints const& /*constraints*/,
    halco::common::typed_array<
        RoutingConstraints::PADIBusConstraints,
        halco::hicann_dls::vx::v3::PADIBusOnDLS> const& padi_bus_constraints,
    std::vector<grenade::common::VertexOnTopology> const& partitioned_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	// All background spike sources act as source because they can't be filtered before reaching
	// their PADI-bus.
	std::vector<SourceOnPADIBusManager::BackgroundSource> background_sources;
	std::vector<std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>>
	    descriptors;
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		std::optional<grenade::common::VertexOnTopology> descriptor;
		for (auto const& partitioned_vertex_descriptor : partitioned_vertex_descriptors) {
			for (auto const& inter_graph_hyper_edge_descriptor :
			     topology.inter_graph_hyper_edges_by_reference(partitioned_vertex_descriptor)) {
				auto const& links = topology.links(inter_graph_hyper_edge_descriptor);
				if (!links.empty()) {
					if (auto const background_spike_source =
					        dynamic_cast<signal_flow::vertex::BackgroundSpikeSource const*>(
					            &topology.get(links.at(0)));
					    background_spike_source) {
						if (background_spike_source->coordinate.toPADIBusOnDLS() == padi_bus) {
							descriptor.emplace(partitioned_vertex_descriptor);
							break;
						}
					}
				}
			}
		}
		if (!descriptor) {
			continue;
		}
		auto const& local_padi_bus_constraints = padi_bus_constraints[padi_bus];
		std::vector<SourceOnPADIBusManager::BackgroundSource> local_background_sources;
		std::vector<
		    std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>>
		    local_descriptors;
		for (size_t i = 0; i < local_padi_bus_constraints.num_background_spike_sources; ++i) {
			SourceOnPADIBusManager::BackgroundSource source;
			source.padi_bus = padi_bus;
			local_background_sources.push_back(source);
		}
		local_descriptors.resize(local_background_sources.size());
		for (size_t i = 0; i < local_descriptors.size(); ++i) {
			local_descriptors.at(i) = std::tuple{*descriptor, i, CompartmentOnLogicalNeuron()};
		}
		for (auto const& connection : local_padi_bus_constraints.background_connections) {
			auto const source_index = std::get<1>(connection.source_descriptor);
			auto& source = local_background_sources.at(source_index);
			if (!source.out_degree.contains(connection.receptor_type)) {
				source.out_degree[connection.receptor_type].fill(0);
			}
			source.out_degree.at(connection.receptor_type)[connection.target]++;
		}
		background_sources.insert(
		    background_sources.end(), local_background_sources.begin(),
		    local_background_sources.end());
		descriptors.insert(descriptors.end(), local_descriptors.begin(), local_descriptors.end());
	}
	LOG4CXX_DEBUG(
	    m_logger, "get_background_sources(): Got " << background_sources.size() << " sources.");
	return std::pair{background_sources, descriptors};
}

std::pair<
    std::vector<SourceOnPADIBusManager::ExternalSource>,
    std::vector<std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>>>
RoutingBuilder::get_external_sources(
    RoutingConstraints const& constraints,
    halco::common::typed_array<
        RoutingConstraints::PADIBusConstraints,
        halco::hicann_dls::vx::v3::PADIBusOnDLS> const& /*padi_bus_constraints*/,
    std::vector<grenade::common::VertexOnTopology> const& partitioned_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	// All external sources act as sources.
	// External sources which don't act as sources to any connection and are not recorded are
	// ignored.
	std::vector<SourceOnPADIBusManager::ExternalSource> external_sources;
	auto const external_connections = constraints.get_external_connections();
	std::set<std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>>
	    external_source_descriptors;
	for (auto const& connection : external_connections) {
		external_source_descriptors.insert(connection.source_descriptor);
	}
	// add recorded sources
	for (auto const& partitioned_vertex_descriptor : partitioned_vertex_descriptors) {
		if (auto const recorder = dynamic_cast<grenade::common::Recorder const*>(
		        &topology.get_reference().get(partitioned_vertex_descriptor));
		    recorder) {
			if (auto const spike_recorder = dynamic_cast<abstract::SpikeRecorder const*>(recorder);
			    spike_recorder) {
				for (auto const& in_edge_descriptor :
				     topology.get_reference().in_edges(partitioned_vertex_descriptor)) {
					auto const source_vertex_descriptor =
					    topology.get_reference().source(in_edge_descriptor);
					auto const& population = dynamic_cast<grenade::common::Population const&>(
					    topology.get_reference().get(
					        topology.get_reference().source(in_edge_descriptor)));
					if (auto const external_source =
					        dynamic_cast<abstract::ExternalSourceNeuron const*>(
					            &population.get_cell());
					    external_source) {
						auto const& in_edge = topology.get_reference().get(in_edge_descriptor);
						auto const local_descriptors = grenade::common::CuboidMultiIndexSequence(
						                                   {population.get_shape().size()})
						                                   .related_sequence_subset_restriction(
						                                       population.get_output_ports()
						                                           .at(in_edge.port_on_source)
						                                           .get_channels(),
						                                       in_edge.get_channels_on_source());
						for (auto const& element : local_descriptors->get_elements()) {
							external_source_descriptors.insert(std::tuple{
							    source_vertex_descriptor, element.value.at(0),
							    CompartmentOnLogicalNeuron()});
						}
					}
				}
			}
		}
		if (auto const population = dynamic_cast<grenade::common::Population const*>(
		        &topology.get_reference().get(partitioned_vertex_descriptor));
		    population) {
			if (auto const external_source =
			        dynamic_cast<abstract::ExternalSourceNeuron const*>(&population->get_cell());
			    external_source) {
				for (auto const& out_edge_descriptor :
				     topology.get_reference().out_edges(partitioned_vertex_descriptor)) {
					auto const target_vertex_descriptor =
					    topology.get_reference().target(out_edge_descriptor);
					auto const& target_execution_instance =
					    dynamic_cast<grenade::common::PartitionedVertex const&>(
					        topology.get_reference().get(target_vertex_descriptor))
					        .get_execution_instance_on_executor()
					        .value();
					if (target_execution_instance ==
					    population->get_execution_instance_on_executor().value()) {
						continue;
					}
					auto const& out_edge = topology.get_reference().get(out_edge_descriptor);
					auto const local_descriptors =
					    grenade::common::CuboidMultiIndexSequence({population->get_shape().size()})
					        .related_sequence_subset_restriction(
					            population->get_output_ports()
					                .at(out_edge.port_on_source)
					                .get_channels(),
					            out_edge.get_channels_on_source());
					for (auto const& element : local_descriptors->get_elements()) {
						external_source_descriptors.insert(std::tuple{
						    partitioned_vertex_descriptor, element.value.at(0),
						    CompartmentOnLogicalNeuron()});
					}
				}
			}
		}
	}
	external_sources.resize(external_source_descriptors.size());
	for (auto const& connection : external_connections) {
		auto const source_index = std::distance(
		    external_source_descriptors.begin(),
		    external_source_descriptors.find(connection.source_descriptor));
		auto& source = external_sources.at(source_index);
		if (!source.out_degree.contains(connection.receptor_type)) {
			source.out_degree[connection.receptor_type].fill(0);
		}
		source.out_degree.at(connection.receptor_type)[connection.target]++;
	}
	auto const descriptors = std::vector<
	    std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>>{
	    external_source_descriptors.begin(), external_source_descriptors.end()};
	LOG4CXX_DEBUG(
	    m_logger, "get_external_sources(): Got " << external_sources.size() << " sources.");
	return std::pair{external_sources, descriptors};
}

std::map<
    std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>,
    halco::hicann_dls::vx::v3::SpikeLabel>
RoutingBuilder::get_internal_labels(
    std::vector<
        std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>> const&
        descriptors,
    SourceOnPADIBusManager::Partition const& partition,
    std::vector<SynapseDriverOnDLSManager::Allocation> const& allocations) const
{
	// The label consists of the label found by the routing and the synapse label.
	// The latter is assigned linearly here, since its assignment does not have any influence on the
	// later steps, it only has to be unambiguous.
	std::map<
	    std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>,
	    halco::hicann_dls::vx::v3::SpikeLabel>
	    labels;
	for (size_t i = 0; i < partition.internal.size(); ++i) {
		auto const local_label = allocations.at(i).label;
		auto const& local_sources = partition.internal.at(i).sources;
		for (size_t k = 0; k < local_sources.size(); ++k) {
			auto const local_index = local_sources.at(k);
			auto const& local_descriptor = descriptors.at(local_index);
			assert(!labels.contains(local_descriptor));
			halco::hicann_dls::vx::v3::SpikeLabel label;
			label.set_row_select_address(haldls::vx::v3::PADIEvent::RowSelectAddress(local_label));
			label.set_synapse_label(halco::hicann_dls::vx::SynapseLabel(k));
			labels[local_descriptor] = label;
		}
	}
	return labels;
}

std::map<
    std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>,
    std::map<HemisphereOnDLS, halco::hicann_dls::vx::v3::SpikeLabel>>
RoutingBuilder::get_background_labels(
    std::vector<
        std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>> const&
        descriptors,
    std::vector<SourceOnPADIBusManager::BackgroundSource> const& background_sources,
    SourceOnPADIBusManager::Partition const& partition,
    std::vector<SynapseDriverOnDLSManager::Allocation> const& allocations,
    grenade::common::LinkedTopology const& topology) const
{
	std::set<std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>>
	    recorded_sources;
	for (auto const& [vertex_descriptor, neuron_on_vertex, compartment_on_neuron] : descriptors) {
		for (auto const& out_edge_descriptor :
		     topology.get_reference().out_edges(vertex_descriptor)) {
			auto const source_vertex_descriptor =
			    topology.get_reference().target(out_edge_descriptor);
			if (auto const recorder = dynamic_cast<abstract::SpikeRecorder const*>(
			        &topology.get_reference().get(source_vertex_descriptor));
			    recorder) {
				auto const& out_edge = topology.get_reference().get(out_edge_descriptor);
				auto const& population = dynamic_cast<grenade::common::Population const&>(
				    topology.get_reference().get(vertex_descriptor));
				auto const neurons =
				    grenade::common::CuboidMultiIndexSequence({population.get_shape().size()})
				        .related_sequence_subset_restriction(
				            population.get_output_ports()
				                .at(out_edge.port_on_source)
				                .get_channels(),
				            out_edge.get_channels_on_source());
				for (auto const& element : neurons->get_elements()) {
					recorded_sources.insert(
					    {vertex_descriptor, element.value.at(0), CompartmentOnLogicalNeuron()});
				}
			} else {
				// add inter-execution-instance sources, as they need to be recorded
				auto const& population = dynamic_cast<grenade::common::Population const&>(
				    topology.get_reference().get(vertex_descriptor));
				auto const target_vertex_descriptor =
				    topology.get_reference().target(out_edge_descriptor);
				auto const& target_execution_instance =
				    dynamic_cast<grenade::common::PartitionedVertex const&>(
				        topology.get_reference().get(target_vertex_descriptor))
				        .get_execution_instance_on_executor()
				        .value();
				if (target_execution_instance ==
				    population.get_execution_instance_on_executor().value()) {
					continue;
				}
				auto const& out_edge = topology.get_reference().get(out_edge_descriptor);
				auto const local_descriptors =
				    grenade::common::CuboidMultiIndexSequence({population.get_shape().size()})
				        .related_sequence_subset_restriction(
				            population.get_output_ports()
				                .at(out_edge.port_on_source)
				                .get_channels(),
				            out_edge.get_channels_on_source());
				for (auto const& element : local_descriptors->get_elements()) {
					recorded_sources.insert(std::tuple{
					    vertex_descriptor, element.value.at(0), CompartmentOnLogicalNeuron()});
				}
			}
		}
	}

	// The label consists of the label found by the routing and the synapse label.
	// The latter is assigned linearly here, since its assignment does not have any influence on the
	// later steps, it only has to be unambiguous.
	std::map<
	    std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>,
	    std::map<HemisphereOnDLS, halco::hicann_dls::vx::v3::SpikeLabel>>
	    labels;
	for (size_t i = 0; i < partition.background.size(); ++i) {
		auto const local_label = allocations.at(partition.internal.size() + i).label;
		auto const& local_sources = partition.background.at(i).sources;
		for (size_t k = 0; k < local_sources.size(); ++k) {
			auto const local_index = local_sources.at(k);
			auto const& hemisphere = background_sources.at(local_index)
			                             .padi_bus.toPADIBusBlockOnDLS()
			                             .toHemisphereOnDLS();
			auto const& local_descriptor = descriptors.at(local_index);
			assert(
			    !labels.contains(local_descriptor) ||
			    !labels.at(local_descriptor).contains(hemisphere));
			// FIXME: also support only one background source on chip again (which might also be the
			// bottom one)
			bool const enable_record_spikes =
			    recorded_sources.contains(descriptors.at(i)) && hemisphere == HemisphereOnDLS::top;
			halco::hicann_dls::vx::v3::SpikeLabel label;
			label.set_neuron_label(NeuronLabel(
			    (enable_record_spikes ? 1 << 11 // used for recording background sources
			                          : 0) +
			    (background_sources.at(local_index).padi_bus.toPADIBusBlockOnDLS() ==
			             PADIBusBlockOnDLS::top
			         ? 1 << 13 // used for distinguishing hemispheres
			         : 0)));
			label.set_row_select_address(haldls::vx::v3::PADIEvent::RowSelectAddress(local_label));
			label.set_synapse_label(halco::hicann_dls::vx::SynapseLabel(k));
			label.set_spl1_address(SPL1Address(
			    background_sources.at(local_index).padi_bus.toPADIBusOnPADIBusBlock().value()));
			labels[local_descriptor][hemisphere] = label;
		}
	}
	return labels;
}

std::map<
    std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>,
    std::map<PADIBusOnDLS, halco::hicann_dls::vx::v3::SpikeLabel>>
RoutingBuilder::get_external_labels(
    std::vector<
        std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>> const&
        descriptors,
    SourceOnPADIBusManager::Partition const& partition,
    std::vector<SynapseDriverOnDLSManager::Allocation> const& allocations,
    grenade::common::LinkedTopology const& topology) const
{
	std::set<std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>>
	    recorded_sources;
	for (auto const& [vertex_descriptor, neuron_on_vertex, compartment_on_neuron] : descriptors) {
		for (auto const& out_edge_descriptor :
		     topology.get_reference().out_edges(vertex_descriptor)) {
			auto const target_vertex_descriptor =
			    topology.get_reference().target(out_edge_descriptor);
			auto const& population = dynamic_cast<grenade::common::Population const&>(
			    topology.get_reference().get(vertex_descriptor));
			if (auto const recorder = dynamic_cast<abstract::SpikeRecorder const*>(
			        &topology.get_reference().get(target_vertex_descriptor));
			    recorder ||
			    dynamic_cast<grenade::common::PartitionedVertex const&>(
			        topology.get_reference().get(target_vertex_descriptor))
			            .get_execution_instance_on_executor()
			            .value() != population.get_execution_instance_on_executor().value()) {
				auto const& out_edge = topology.get_reference().get(out_edge_descriptor);
				auto const neurons =
				    grenade::common::CuboidMultiIndexSequence({population.get_shape().size()})
				        .related_sequence_subset_restriction(
				            population.get_output_ports()
				                .at(out_edge.port_on_source)
				                .get_channels(),
				            out_edge.get_channels_on_source());
				for (auto const& element : neurons->get_elements()) {
					recorded_sources.insert(
					    {vertex_descriptor, element.value.at(0), CompartmentOnLogicalNeuron()});
				}
			}
		}
	}

	// The label consists of the label found by the routing, the synapse label and the static label
	// part used for filtering in the crossbar. The synapse label is assigned linearly here, since
	// its assignment does not have any influence on the later steps, it only has to be unambiguous.
	// The static label part used for crossbar filtering is aligned to the requirements in
	// apply_crossbar_internal().
	std::map<
	    std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>,
	    std::map<PADIBusOnDLS, halco::hicann_dls::vx::v3::SpikeLabel>>
	    labels;
	for (size_t i = 0; i < partition.external.size(); ++i) {
		auto const& local_allocation =
		    allocations.at(partition.internal.size() + partition.background.size() + i);
		auto const local_label = local_allocation.label;
		auto const targets = local_allocation.synapse_drivers;
		auto const& local_sources = partition.external.at(i).sources;
		for (size_t k = 0; k < local_sources.size(); ++k) {
			auto const local_index = local_sources.at(k);
			auto const& local_descriptor = descriptors.at(local_index);
			for (auto const& [padi_bus, _] : targets) {
				halco::hicann_dls::vx::v3::SpikeLabel label;
				label.set_neuron_label(NeuronLabel(
				    (padi_bus.toPADIBusBlockOnDLS().value() << 13) |
				    (recorded_sources.contains(local_descriptor)
				         ? 1 << 12 // used for recording external sources
				         : 0)));
				label.set_row_select_address(
				    haldls::vx::v3::PADIEvent::RowSelectAddress(local_label));
				label.set_synapse_label(halco::hicann_dls::vx::SynapseLabel(k));
				label.set_spl1_address(SPL1Address(padi_bus.toPADIBusOnPADIBusBlock().value()));
				labels[local_descriptor][padi_bus] = label;
			}
		}
	}
	return labels;
}

void RoutingBuilder::apply_source_labels(
    RoutingConstraints const& constraints,
    std::map<
        std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>,
        halco::hicann_dls::vx::v3::SpikeLabel> const& internal,
    std::map<
        std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>,
        std::map<HemisphereOnDLS, halco::hicann_dls::vx::v3::SpikeLabel>> const& background,
    std::map<
        std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>,
        std::map<PADIBusOnDLS, halco::hicann_dls::vx::v3::SpikeLabel>> const& external,
    grenade::common::LinkedTopology const& topology,
    std::vector<grenade::common::VertexOnTopology> const& partitioned_vertex_descriptors,
    Result& result) const
{
	auto const anchors = constraints.get_anchors();
	auto const neither_recorded_nor_source_neurons =
	    constraints.get_neither_recorded_nor_source_neurons();
	std::vector<
	    std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron, size_t>>
	    unset_labels;
	for (auto const& partitioned_vertex_descriptor : partitioned_vertex_descriptors) {
		if (auto const population = dynamic_cast<grenade::common::Population const*>(
		        &topology.get_reference().get(partitioned_vertex_descriptor));
		    population) {
			if (auto const locally_placed_neuron =
			        dynamic_cast<abstract::LocallyPlacedNeuron const*>(&population->get_cell());
			    locally_placed_neuron) {
				auto& local_labels = result.internal_neuron_labels[partitioned_vertex_descriptor];
				local_labels.resize(population->get_shape().size());
				for (size_t i = 0; i < local_labels.size(); ++i) {
					for (auto const& [compartment, atomic_neurons] :
					     LogicalNeuronOnDLS(
					         locally_placed_neuron->shape,
					         anchors.at(partitioned_vertex_descriptor).at(i))
					         .get_placed_compartments()) {
						for (size_t an = 0; an < atomic_neurons.size(); ++an) {
							local_labels.at(i)[compartment].resize(atomic_neurons.size());
							if (!internal.contains(
							        std::tuple{partitioned_vertex_descriptor, i, compartment})) {
								if (!neither_recorded_nor_source_neurons.contains(
								        atomic_neurons.at(an))) {
									LOG4CXX_DEBUG(
									    m_logger, "apply_source_labels(): Unset label for: "
									                  << partitioned_vertex_descriptor << " " << i
									                  << " " << compartment << " " << an << ".");
									auto const& spike_master =
									    locally_placed_neuron->compartments
									        .at(grenade::common::CompartmentOnNeuron(compartment))
									        .spike_master;
									if (!spike_master ||
									    (spike_master->atomic_neuron_on_compartment != an)) {
										throw std::logic_error(
										    "Atomic neuron can't be either recorded or source "
										    "neuron "
										    "but not feature a spike master or not be the spike "
										    "master.");
									}
									unset_labels.push_back(std::tuple{
									    partitioned_vertex_descriptor, i, compartment, an});
								} // else no label is needed
							} else {
								[[maybe_unused]] auto const& spike_master =
								    locally_placed_neuron->compartments
								        .at(grenade::common::CompartmentOnNeuron(compartment))
								        .spike_master;
								assert(spike_master);
								if (spike_master->atomic_neuron_on_compartment == an) {
									local_labels.at(i)[compartment].at(an).emplace(
									    internal
									        .at(std::tuple{
									            partitioned_vertex_descriptor, i, compartment})
									        .get_neuron_backend_address_out());
									LOG4CXX_DEBUG(
									    m_logger, "apply_source_labels(): Set label for: "
									                  << partitioned_vertex_descriptor << " " << i
									                  << " " << compartment << " " << an << ".");
								}
							}
						}
					}
				}
			} else if (auto const external_source_neuron =
			               dynamic_cast<abstract::ExternalSourceNeuron const*>(
			                   &population->get_cell());
			           external_source_neuron) {
				auto& local_labels = result.external_spike_labels[partitioned_vertex_descriptor];
				local_labels.resize(population->get_shape().size());
				for (size_t i = 0; i < local_labels.size(); ++i) {
					if (!external.contains(std::tuple{
					        partitioned_vertex_descriptor, i, CompartmentOnLogicalNeuron()})) {
						// TODO: This might happen and in principle is not an error.
					} else {
						for (auto const& [_, l] : external.at(std::tuple{
						         partitioned_vertex_descriptor, i, CompartmentOnLogicalNeuron()})) {
							local_labels.at(i).push_back(l);
						}
					}
				}
			} else if (auto const poisson_source_neuron =
			               dynamic_cast<abstract::PoissonSourceNeuron const*>(
			                   &population->get_cell());
			           poisson_source_neuron) {
				auto const size = population->get_shape().size();
				for (size_t i = 0; i < size; ++i) {
					if (!background.contains(std::tuple{
					        partitioned_vertex_descriptor, i, CompartmentOnLogicalNeuron()})) {
						// This shall never happen, since all neurons in a background population are
						// treated as sources because they can't be filtered from reaching their
						// PADI-bus.
						throw std::logic_error("Background spike source label unknown.");
					} else {
						for (auto [hemisphere, label] : background.at(std::tuple{
						         partitioned_vertex_descriptor, i, CompartmentOnLogicalNeuron()})) {
							result
							    .background_spike_source_labels[partitioned_vertex_descriptor]
							                                   [hemisphere]
							    .push_back(label);
						}
					}
				}
				for (auto const& [hemisphere, _] :
				     result.background_spike_source_labels.at(partitioned_vertex_descriptor)) {
					result
					    .background_spike_source_masks[partitioned_vertex_descriptor][hemisphere] =
					    haldls::vx::v3::BackgroundSpikeSource::Mask(size - 1);
				}
				// TODO(SpikeIO)
				//
				//		} else if (std::holds_alternative<SpikeIOSourcePopulation>(population)) {
				//			auto& local_labels = result.external_spike_labels[descriptor];
				//			local_labels.resize(std::get<SpikeIOSourcePopulation>(population).neurons.size());
				//			for (size_t i = 0; i < local_labels.size(); ++i) {
				//				if (!external.contains(std::tuple{descriptor, i,
				// CompartmentOnLogicalNeuron()})) {
				//					// This might happen, but is in principle not an error.
				//				} else {
				//					for (auto const& [_, l] :
				//					     external.at(std::tuple{descriptor, i,
				// CompartmentOnLogicalNeuron()})) {
				// local_labels.at(i).push_back(l);
				//					}
				//				}
				//			}
				//		}
				//	}
				//
			} else {
				throw std::logic_error("Population type not implemented.");
			}
		}
	}
	// add external labels for populations on other execution instances
	for (auto const& [source, labels] : external) {
		auto const& [partitioned_vertex_descriptor, neuron_on_population, compartment_on_neuron] =
		    source;
		if (std::find(
		        partitioned_vertex_descriptors.begin(), partitioned_vertex_descriptors.end(),
		        partitioned_vertex_descriptor) != partitioned_vertex_descriptors.end()) {
			continue;
		}
		auto& local_labels = result.external_spike_labels[partitioned_vertex_descriptor];
		auto const& population = dynamic_cast<grenade::common::Population const&>(
		    topology.get_reference().get(partitioned_vertex_descriptor));
		local_labels.resize(population.get_shape().size());
		for (auto const& [_, l] : labels) {
			local_labels.at(neuron_on_population).push_back(l);
		}
	}
	// all unset labels can be uniquely assigned now
	std::set<halco::hicann_dls::vx::v3::SpikeLabel> set_labels;
	for (auto const& [_, label] : internal) {
		set_labels.insert(label);
	}
	for (auto const& [descriptor, index, compartment, neuron_on_compartment] : unset_labels) {
		auto const& neuron = dynamic_cast<abstract::LocallyPlacedNeuron const&>(
		    dynamic_cast<grenade::common::Population const&>(
		        topology.get_reference().get(descriptor))
		        .get_cell());
		halco::hicann_dls::vx::v3::SpikeLabel label;
		label.set_neuron_event_output(
		    LogicalNeuronOnDLS(neuron.shape, anchors.at(descriptor).at(index))
		        .get_placed_compartments()
		        .at(compartment)
		        .at(neuron_on_compartment)
		        .toNeuronColumnOnDLS()
		        .toNeuronEventOutputOnDLS());
		bool success = false;
		for (auto const neuron_backend_address_out :
		     iter_all<halco::hicann_dls::vx::NeuronBackendAddressOut>()) {
			label.set_neuron_backend_address_out(neuron_backend_address_out);
			if (!set_labels.contains(label)) {
				auto& local_labels = result.internal_neuron_labels[descriptor];
				local_labels.at(index)[compartment].at(neuron_on_compartment) =
				    label.get_neuron_backend_address_out();
				set_labels.insert(label);
				success = true;
				break;
			}
		}
		if (!success) {
			std::stringstream ss;
			ss << "No label for (" << descriptor << ", " << index << ", " << compartment << ", "
			   << neuron_on_compartment << ") was found.";
			throw std::logic_error(ss.str());
		}
	}
}

std::map<
    std::pair<grenade::common::VertexOnTopology, size_t>,
    std::vector<RoutingBuilder::PlacedConnection>>
RoutingBuilder::place_routed_connections(
    std::vector<RoutedConnection> const& connections,
    std::vector<halco::hicann_dls::vx::v3::SynapseRowOnDLS> const& synapse_rows) const
{
	// We place connections to a set of rows by filling like:
	// x   x     x
	// x   x x   x
	// x x x x x x
	std::map<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, size_t> num_placed_connections_by_target;
	std::map<
	    std::pair<grenade::common::VertexOnTopology, size_t>,
	    std::vector<RoutingBuilder::PlacedConnection>>
	    ret;
	for (auto const& connection : connections) {
		auto const num_connections = num_placed_connections_by_target[connection.target];
		PlacedConnection placed_connection{
		    synapse_rows.at(num_connections),
		    connection.target.toNeuronColumnOnDLS().toSynapseOnSynapseRow(),
		    connection.source_descriptor};
		ret[connection.descriptor].push_back(placed_connection);
		num_placed_connections_by_target.at(connection.target)++;
	}
	return ret;
}

template <typename Connection>
std::map<
    std::pair<grenade::common::VertexOnTopology, size_t>,
    std::vector<RoutingBuilder::PlacedConnection>>
RoutingBuilder::place_routed_connections(
    std::vector<Connection> const& connections,
    std::map<Receptor::Type, std::vector<halco::hicann_dls::vx::v3::SynapseRowOnDLS>> const&
        synapse_rows) const
{
	std::map<
	    std::pair<grenade::common::VertexOnTopology, size_t>,
	    std::vector<RoutingBuilder::PlacedConnection>>
	    ret;
	for (auto const& [receptor_type, rows] : synapse_rows) {
		std::vector<RoutedConnection> routed_connections;
		for (auto const& connection : connections) {
			if (connection.receptor_type == receptor_type) {
				routed_connections.push_back(RoutedConnection{
				    connection.descriptor, connection.target, connection.source_descriptor});
			}
		}
		auto placed_connections = place_routed_connections(routed_connections, rows);
		ret.merge(placed_connections);
		assert(placed_connections.empty());
	}
	return ret;
}

template <typename Sources>
std::map<
    std::pair<grenade::common::VertexOnTopology, size_t>,
    std::vector<RoutingBuilder::PlacedConnection>>
RoutingBuilder::place_routed_connections(
    std::vector<SourceOnPADIBusManager::Partition::Group> const& partition,
    std::vector<
        std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>> const&
        descriptors,
    Sources const& sources,
    std::vector<SynapseDriverOnDLSManager::Allocation> const& padi_bus_allocations,
    size_t offset,
    RoutingConstraints const& constraints,
    Result& result) const
{
	std::map<std::pair<grenade::common::VertexOnTopology, size_t>, std::vector<PlacedConnection>>
	    ret;
	for (size_t i = 0; i < partition.size(); ++i) {
		auto const& local_allocation = padi_bus_allocations.at(i + offset);
		auto const& local_sources = partition.at(i).sources;
		std::set<std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>>
		    local_descriptors;
		for (auto const& ls : local_sources) {
			local_descriptors.insert(descriptors.at(ls));
		}
		auto const local_synapse_drivers = local_allocation.synapse_drivers;
		std::map<
		    Receptor::Type,
		    halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>>
		    in_degree;
		in_degree[Receptor::Type::excitatory].fill(0);
		in_degree[Receptor::Type::inhibitory].fill(0);

		for (size_t k = 0; k < local_sources.size(); ++k) {
			auto const local_index = local_sources.at(k);
			auto const& local_source = sources.at(local_index);
			for (auto const& [receptor_type, out_degree] : local_source.out_degree) {
				for (auto const an : iter_all<AtomicNeuronOnDLS>()) {
					in_degree[receptor_type][an] += out_degree[an];
				}
			}
		}

		std::map<Receptor::Type, typed_array<size_t, PADIBusBlockOnDLS>> num_synapse_rows;
		num_synapse_rows[Receptor::Type::excitatory].fill(0);
		num_synapse_rows[Receptor::Type::inhibitory].fill(0);
		for (auto const& [receptor_type, in] : in_degree) {
			for (auto const an : iter_all<AtomicNeuronOnDLS>()) {
				auto& local_num_synapse_rows =
				    num_synapse_rows[receptor_type][an.toNeuronRowOnDLS().toPADIBusBlockOnDLS()];
				local_num_synapse_rows = std::max(local_num_synapse_rows, in[an]);
			}
		}

		std::map<PADIBusOnDLS, std::map<Receptor::Type, std::vector<SynapseRowOnDLS>>> synapse_rows;
		for (auto const& [padi_bus, synapse_drivers] : local_synapse_drivers) {
			std::vector<SynapseRowOnDLS> local_synapse_rows;
			// FIXME: support more shapes than one
			for (auto const& [synapse_driver, mask] : synapse_drivers.synapse_drivers.at(0)) {
				for (auto const r : iter_all<SynapseRowOnSynapseDriver>()) {
					SynapseRowOnDLS synapse_row(
					    synapse_driver
					        .toSynapseDriverOnSynapseDriverBlock()[padi_bus
					                                                   .toPADIBusOnPADIBusBlock()]
					        .toSynapseRowOnSynram()[r],
					    padi_bus.toPADIBusBlockOnDLS().toSynramOnDLS());
					local_synapse_rows.push_back(synapse_row);
					SynapseDriverOnDLS global_synapse_driver(
					    synapse_driver.toSynapseDriverOnSynapseDriverBlock()
					        [padi_bus.toPADIBusOnPADIBusBlock()],
					    padi_bus.toPADIBusBlockOnDLS().toSynapseDriverBlockOnDLS());
					result.synapse_driver_compare_masks[global_synapse_driver] = mask;
				}
			}
			std::sort(local_synapse_rows.begin(), local_synapse_rows.end());
			size_t o = 0;
			for (auto const& [receptor_type, num] : num_synapse_rows) {
				for (size_t j = 0; j < num[padi_bus.toPADIBusBlockOnDLS()]; ++j) {
					synapse_rows[padi_bus][receptor_type].push_back(local_synapse_rows.at(o + j));
				}
				o += num[padi_bus.toPADIBusBlockOnDLS()];
			}
		}
		for (auto const& [padi_bus, rows] : synapse_rows) {
			for (auto const& [receptor_type, rs] : rows) {
				for (auto const& row : rs) {
					result.synapse_row_modes[row] =
					    receptor_type == Receptor::Type::excitatory
					        ? haldls::vx::v3::SynapseDriverConfig::RowMode::excitatory
					        : haldls::vx::v3::SynapseDriverConfig::RowMode::inhibitory;
					for (auto const orow : iter_all<SynapseRowOnSynapseDriver>()) {
						SynapseRowOnDLS other_row(
						    row.toSynapseRowOnSynram()
						        .toSynapseDriverOnSynapseDriverBlock()
						        .toSynapseRowOnSynram()[orow],
						    row.toSynramOnDLS());
						if (!result.synapse_row_modes.contains(other_row)) {
							result.synapse_row_modes[other_row] =
							    haldls::vx::v3::SynapseDriverConfig::RowMode::disabled;
						}
					}
				}
			}
			auto const padi_bus_constraints = constraints.get_padi_bus_constraints();
			auto internal_connections = padi_bus_constraints[padi_bus].internal_connections;
			auto background_connections = padi_bus_constraints[padi_bus].background_connections;

			auto const filter = [&](auto const& connections) {
				auto ret = connections;
				ret.clear();
				for (auto const& c : connections) {
					if (local_descriptors.contains(c.source_descriptor) &&
					    (c.target.toNeuronRowOnDLS().toPADIBusBlockOnDLS() ==
					     padi_bus.toPADIBusBlockOnDLS())) {
						ret.push_back(c);
					}
				}
				return ret;
			};

			ret.merge(place_routed_connections(
			    filter(padi_bus_constraints[padi_bus].internal_connections), rows));
			ret.merge(place_routed_connections(
			    filter(padi_bus_constraints[padi_bus].background_connections), rows));
			ret.merge(
			    place_routed_connections(filter(constraints.get_external_connections()), rows));
		}
	}
	return ret;
}

std::map<
    std::pair<grenade::common::VertexOnTopology, size_t>,
    std::vector<RoutingBuilder::PlacedConnection>>
RoutingBuilder::place_routed_connections(
    SourceOnPADIBusManager::Partition const& partition,
    std::vector<
        std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>> const&
        internal_descriptors,
    std::vector<
        std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>> const&
        background_descriptors,
    std::vector<
        std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>> const&
        external_descriptors,
    std::vector<SourceOnPADIBusManager::InternalSource> const& internal_sources,
    std::vector<SourceOnPADIBusManager::BackgroundSource> const& background_sources,
    std::vector<SourceOnPADIBusManager::ExternalSource> const& external_sources,
    std::vector<SynapseDriverOnDLSManager::Allocation> const& padi_bus_allocations,
    RoutingConstraints const& constraints,
    Result& result) const
{
	std::map<std::pair<grenade::common::VertexOnTopology, size_t>, std::vector<PlacedConnection>>
	    ret;
	ret.merge(place_routed_connections(
	    partition.internal, internal_descriptors, internal_sources, padi_bus_allocations, 0,
	    constraints, result));
	ret.merge(place_routed_connections(
	    partition.background, background_descriptors, background_sources, padi_bus_allocations,
	    partition.internal.size(), constraints, result));
	ret.merge(place_routed_connections(
	    partition.external, external_descriptors, external_sources, padi_bus_allocations,
	    partition.internal.size() + partition.background.size(), constraints, result));
	return ret;
}

void RoutingBuilder::apply_routed_connections(
    std::map<
        std::pair<grenade::common::VertexOnTopology, size_t>,
        std::vector<PlacedConnection>> const& placed_connections,
    std::map<
        std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>,
        halco::hicann_dls::vx::v3::SpikeLabel> const& internal_labels,
    std::map<
        std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>,
        std::map<HemisphereOnDLS, halco::hicann_dls::vx::v3::SpikeLabel>> const& background_labels,
    std::map<
        std::tuple<grenade::common::VertexOnTopology, size_t, CompartmentOnLogicalNeuron>,
        std::map<PADIBusOnDLS, halco::hicann_dls::vx::v3::SpikeLabel>> const& external_labels,
    grenade::common::LinkedTopology const& topology,
    std::vector<grenade::common::VertexOnTopology> const& partitioned_vertex_descriptors,
    Result& result) const
{
	// Using the placement of the connections and their properties like weight and calculating their
	// label from the source label fully specified plaecd and routed connections are added into the
	// routing result.

	for (auto const& partitioned_vertex_descriptor : partitioned_vertex_descriptors) {
		if (auto const projection = dynamic_cast<grenade::common::Projection const*>(
		        &topology.get_reference().get(partitioned_vertex_descriptor));
		    projection) {
			auto& local_placed_connections = result.connections[partitioned_vertex_descriptor];
			auto const section =
			    projection->get_connector().get_input_sequence()->cartesian_product(
			        *projection->get_connector().get_output_sequence());
			local_placed_connections.resize(projection->get_connector().get_num_synapses(*section));
			for (size_t i = 0; i < local_placed_connections.size(); ++i) {
				auto const& local_placed_connection_list =
				    placed_connections.at(std::pair{partitioned_vertex_descriptor, i});
				for (auto const& placed_connection : local_placed_connection_list) {
					Result::PlacedConnection local_placed_connection;
					auto const synapse_row = placed_connection.synapse_row;
					auto const synapse_on_row = placed_connection.synapse_on_row;
					std::tuple const population_descriptor = placed_connection.source_descriptor;
					lola::vx::v3::SynapseMatrix::Label label;
					if (internal_labels.contains(population_descriptor)) {
						label = internal_labels.at(population_descriptor).get_synapse_label();
					} else if (background_labels.contains(population_descriptor)) {
						label = background_labels.at(population_descriptor)
						            .at(synapse_row.toSynramOnDLS().toHemisphereOnDLS())
						            .get_synapse_label();
					} else if (external_labels.contains(population_descriptor)) {
						label = external_labels.at(population_descriptor)
						            .at(PADIBusOnDLS(
						                synapse_row.toSynapseRowOnSynram()
						                    .toSynapseDriverOnSynapseDriverBlock()
						                    .toPADIBusOnPADIBusBlock(),
						                synapse_row.toSynramOnDLS().toPADIBusBlockOnDLS()))
						            .get_synapse_label();
					} else {
						throw std::logic_error("Source label not found.");
					}
					local_placed_connections.at(i).push_back(
					    Result::PlacedConnection{label, synapse_row, synapse_on_row});
				}
			}
		}
	}
}

void RoutingBuilder::apply_crossbar_nodes_from_l2(Result& result) const
{
	// Unique and exclusive targeting of a single PADIBusOnDLS via an external label is the goal.
	// An external source which targets multiple PADIBusOnDLS is enabled to do so by sending
	// multiple labels. This is achieved by diagonal 1:1 correspondence from SPL1Address to
	// PADIBusOnPADIBusBlock. The PADIBusBlockOnDLS then is selected by the highest bit in the
	// 14-bit NeuronLabel. These assumptions are reflected in the assignment of label values for
	// external sources in get_external_labels().

	// disable non-diagonal input from L2
	for (auto const cinput : iter_all<SPL1Address>()) {
		for (auto const coutput : iter_all<PADIBusOnDLS>()) {
			CrossbarNodeOnDLS const coord(
			    coutput.toCrossbarOutputOnDLS(), cinput.toCrossbarInputOnDLS());
			result.crossbar_nodes[coord] = haldls::vx::v3::CrossbarNode::drop_all;
		}
	}
	// enable input from L2 to top half
	for (auto const coutput : iter_all<PADIBusOnPADIBusBlock>()) {
		CrossbarNodeOnDLS const coord(
		    PADIBusOnDLS(coutput, PADIBusBlockOnDLS::top).toCrossbarOutputOnDLS(),
		    SPL1Address(coutput).toCrossbarInputOnDLS());
		haldls::vx::v3::CrossbarNode config;
		config.set_mask(NeuronLabel(1 << 13));
		config.set_target(NeuronLabel(0 << 13));
		result.crossbar_nodes[coord] = config;
	}
	// enable input from L2 to bottom half
	for (auto const coutput : iter_all<PADIBusOnPADIBusBlock>()) {
		CrossbarNodeOnDLS const coord(
		    PADIBusOnDLS(coutput, PADIBusBlockOnDLS::bottom).toCrossbarOutputOnDLS(),
		    SPL1Address(coutput).toCrossbarInputOnDLS());
		haldls::vx::v3::CrossbarNode config;
		config.set_mask(NeuronLabel(1 << 13));
		config.set_target(NeuronLabel(1 << 13));
		result.crossbar_nodes[coord] = config;
	}
}

void RoutingBuilder::apply_crossbar_nodes_from_background(Result& result) const
{
	// No filtering necessary (and possible) for background sources.

	// enable input from background sources
	for (auto const background : iter_all<BackgroundSpikeSourceOnDLS>()) {
		CrossbarNodeOnDLS const coord(
		    background.toPADIBusOnDLS().toCrossbarOutputOnDLS(), background.toCrossbarInputOnDLS());
		result.crossbar_nodes[coord] = haldls::vx::v3::CrossbarNode();
	}
}

void RoutingBuilder::apply_crossbar_nodes_from_background_to_l2(Result& result) const
{
	// Event filtering is performed on the host computer.

	// enable L2 output for events which have bit 11 set and shall be recorded
	for (auto const cinput : iter_all<BackgroundSpikeSourceOnDLS>()) {
		CrossbarNodeOnDLS const coord(
		    cinput.toCrossbarL2OutputOnDLS().toCrossbarOutputOnDLS(),
		    cinput.toCrossbarInputOnDLS());
		result.crossbar_nodes[coord].set_target(NeuronLabel(1 << 11));
		result.crossbar_nodes[coord].set_mask(NeuronLabel(1 << 11));
	}
}

void RoutingBuilder::apply_crossbar_nodes_internal(Result& result) const
{
	// Beforehand, nodes which shall filter all events were disabled in route_internal_crossbar().
	// Therefore here we enable all remaining nodes without filter.

	// enable recurrent connections
	for (auto const cinput : iter_all<NeuronEventOutputOnDLS>()) {
		CrossbarNodeOnDLS const coord(
		    CrossbarOutputOnDLS(cinput % PADIBusOnPADIBusBlock::size),
		    cinput.toCrossbarInputOnDLS());
		if (!result.crossbar_nodes.contains(coord)) {
			result.crossbar_nodes[coord] = haldls::vx::v3::CrossbarNode();
		}
	}
	for (auto const cinput : iter_all<NeuronEventOutputOnDLS>()) {
		CrossbarNodeOnDLS const coord(
		    CrossbarOutputOnDLS(cinput % PADIBusOnPADIBusBlock::size + PADIBusOnPADIBusBlock::size),
		    cinput.toCrossbarInputOnDLS());
		if (!result.crossbar_nodes.contains(coord)) {
			result.crossbar_nodes[coord] = haldls::vx::v3::CrossbarNode();
		}
	}
}

void RoutingBuilder::apply_crossbar_nodes_from_internal_to_l2(Result& result) const
{
	// Event filtering is performed on the host computer.

	// enable L2 output
	for (auto const cinput : iter_all<NeuronEventOutputOnDLS>()) {
		CrossbarNodeOnDLS const coord(
		    CrossbarOutputOnDLS(PADIBusOnDLS::size + (cinput % PADIBusOnPADIBusBlock::size)),
		    cinput.toCrossbarInputOnDLS());
		result.crossbar_nodes[coord] = haldls::vx::v3::CrossbarNode();
	}
}

void RoutingBuilder::apply_crossbar_nodes_from_l2_to_l2(Result& result) const
{
	// Event filtering is performed on the host computer.

	// enable L2 output for events which have bit 12 set and shall be recorded
	for (auto const cinput : iter_all<SPL1Address>()) {
		CrossbarNodeOnDLS const coord(
		    CrossbarOutputOnDLS(PADIBusOnDLS::size + cinput), cinput.toCrossbarInputOnDLS());
		result.crossbar_nodes[coord].set_target(NeuronLabel(1 << 12));
		result.crossbar_nodes[coord].set_mask(NeuronLabel(1 << 12));
	}
}

RoutingBuilder::Result RoutingBuilder::route(
    grenade::common::LinkedTopology const& topology,
    std::vector<grenade::common::VertexOnTopology> const& partitioned_vertex_descriptors,
    ConnectionRoutingResult const& connection_routing_result,
    std::optional<GreedyRouter::Options> const& options) const
{
	hate::Timer timer;
	LOG4CXX_DEBUG(m_logger, "route(): Starting routing.");

	if (options) {
		LOG4CXX_INFO(m_logger, "route(): Using " << *options << ".");
	}

	Result result;
	result.connection_routing_result = connection_routing_result;
	RoutingConstraints const constraints(
	    topology, partitioned_vertex_descriptors, result.connection_routing_result);
	auto padi_bus_constraints = constraints.get_padi_bus_constraints();

	// route internal crossbar nodes between neurons
	SourceOnPADIBusManager::DisabledInternalRoutes disabled_internal_routes;
	route_internal_crossbar(disabled_internal_routes, constraints, padi_bus_constraints, result);

	// get sources
	auto const [internal_sources, internal_descriptors] =
	    get_internal_sources(constraints, padi_bus_constraints);
	auto const [background_sources, background_descriptors] = get_background_sources(
	    constraints, padi_bus_constraints, partitioned_vertex_descriptors, topology);
	auto const [external_sources, external_descriptors] = get_external_sources(
	    constraints, padi_bus_constraints, partitioned_vertex_descriptors, topology);

	// partition sources on PADI-busses
	SourceOnPADIBusManager source_manager(disabled_internal_routes);
	auto const maybe_source_partition =
	    source_manager.solve(internal_sources, background_sources, external_sources);
	if (!maybe_source_partition) {
		throw UnsuccessfulRouting(
		    "Source distribution not successful. This can happen either if internal and background "
		    "sources require more than the available synapse drivers on a single PADI-bus or if no "
		    "distribution of external sources over the PADI-busses is found while using at most "
		    "the available synapse drivers.");
	}
	auto const& source_partition = *maybe_source_partition;

	// flatten allocation properties for SynapseDriverOnDLSManager
	std::vector<SynapseDriverOnDLSManager::AllocationRequest> synapse_driver_allocation_requests;
	for (auto const& internal : source_partition.internal) {
		synapse_driver_allocation_requests.push_back(internal.allocation_request);
	}
	for (auto const& background : source_partition.background) {
		synapse_driver_allocation_requests.push_back(background.allocation_request);
	}
	for (auto const& external : source_partition.external) {
		synapse_driver_allocation_requests.push_back(external.allocation_request);
	}

	// allocate synapse drivers on PADI-bus(ses)
	SynapseDriverOnDLSManager synapse_driver_manager;
	auto const synapse_driver_allocations =
	    options ? synapse_driver_manager.solve(
	                  synapse_driver_allocation_requests, options->synapse_driver_allocation_policy,
	                  options->synapse_driver_allocation_timeout)
	            : synapse_driver_manager.solve(synapse_driver_allocation_requests);

	if (!synapse_driver_allocations) {
		throw UnsuccessfulRouting("Synapse driver allocations not successful.");
	}

	// calculate labels for sources
	auto const internal_labels =
	    get_internal_labels(internal_descriptors, source_partition, *synapse_driver_allocations);
	auto const background_labels = get_background_labels(
	    background_descriptors, background_sources, source_partition, *synapse_driver_allocations,
	    topology);
	auto const external_labels = get_external_labels(
	    external_descriptors, source_partition, *synapse_driver_allocations, topology);

	// add source labels to result
	apply_source_labels(
	    constraints, internal_labels, background_labels, external_labels, topology,
	    partitioned_vertex_descriptors, result);

	// place routed connections onto synapse matrix
	auto const placed_connections = place_routed_connections(
	    source_partition, internal_descriptors, background_descriptors, external_descriptors,
	    internal_sources, background_sources, external_sources, *synapse_driver_allocations,
	    constraints, result);

	// apply routed connections to result
	apply_routed_connections(
	    placed_connections, internal_labels, background_labels, external_labels, topology,
	    partitioned_vertex_descriptors, result);

	// calculate (remaining) crossbar node config
	apply_crossbar_nodes_internal(result);
	apply_crossbar_nodes_from_internal_to_l2(result);
	apply_crossbar_nodes_from_l2_to_l2(result);
	apply_crossbar_nodes_from_l2(result);
	apply_crossbar_nodes_from_background(result);
	apply_crossbar_nodes_from_background_to_l2(result);

	LOG4CXX_DEBUG(m_logger, "route(): Got result: " << result);
	LOG4CXX_TRACE(m_logger, "route(): Finished routing in " << timer.print());
	return result;
}

} // namespace grenade::vx::network::routing::greedy
