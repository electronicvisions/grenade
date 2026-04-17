#include "grenade/vx/network/abstract/mapped_topology_rewrite/routing.h"

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/inter_topology_hyper_edge/fixture.h"
#include "grenade/common/inter_topology_hyper_edge_on_linked_topology.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/population.h"
#include "grenade/common/projection.h"
#include "grenade/common/recorder.h"
#include "grenade/common/resource_estimator.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/mapping/cadc_recorder.h"
#include "grenade/vx/network/abstract/mapping/calibrated_neuron.h"
#include "grenade/vx/network/abstract/mapping/delay_cell.h"
#include "grenade/vx/network/abstract/mapping/external_source_neuron.h"
#include "grenade/vx/network/abstract/mapping/locally_placed_neuron.h"
#include "grenade/vx/network/abstract/mapping/madc_recorder.h"
#include "grenade/vx/network/abstract/mapping/pad_recorder.h"
#include "grenade/vx/network/abstract/mapping/plasticity_rule.h"
#include "grenade/vx/network/abstract/mapping/poisson_source_neuron.h"
#include "grenade/vx/network/abstract/mapping/spike_recorder.h"
#include "grenade/vx/network/abstract/mapping/uncalibrated_neuron.h"
#include "grenade/vx/network/abstract/mapping/uncalibrated_signed_synapse.h"
#include "grenade/vx/network/abstract/mapping/uncalibrated_synapse.h"
#include "grenade/vx/network/abstract/plasticity_rule.h"
#include "grenade/vx/network/abstract/population_cell/calibrated.h"
#include "grenade/vx/network/abstract/population_cell/delay.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/network/abstract/population_cell/poisson_source.h"
#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated_signed.h"
#include "grenade/vx/network/abstract/recorder/cadc.h"
#include "grenade/vx/network/abstract/recorder/madc.h"
#include "grenade/vx/network/abstract/recorder/pad.h"
#include "grenade/vx/network/abstract/recorder/spike.h"
#include "grenade/vx/network/vertex/transformation/spike_lookup.h"
#include "grenade/vx/signal_flow/vertex/background_spike_source.h"
#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_input.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include "grenade/vx/signal_flow/vertex/madc_readout.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "grenade/vx/signal_flow/vertex/pad_readout.h"
#include "grenade/vx/signal_flow/vertex/padi_bus.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view_sparse.h"
#include "grenade/vx/signal_flow/vertex/synapse_driver.h"
#include "halco/hicann-dls/vx/v3/background.h"
#include <ctime>
#include <memory>
#include <stdexcept>
#include <boost/range/iterator_range_core.hpp>

namespace grenade::vx::network::abstract {

RoutingRewrite::RoutingRewrite(
    RoutingResult routing_result, std::shared_ptr<grenade::common::LinkedTopology> topology) :
    TopologyRewrite(std::move(topology)), routing_result(std::move(routing_result))
{
}

void RoutingRewrite::operator()() const
{
	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::vector<grenade::common::VertexOnTopology>>
	    partitioned_vertices_per_execution_instance;

	std::map<grenade::common::ExecutionInstanceOnExecutor, grenade::common::TimeDomainOnTopology>
	    m_time_domains;

	for (auto const& vertex_descriptor : get_topology().get_reference().vertices()) {
		auto const& partitioned_vertex = dynamic_cast<grenade::common::PartitionedVertex const&>(
		    get_topology().get_reference().get(vertex_descriptor));
		auto const& execution_instance_on_executor =
		    partitioned_vertex.get_execution_instance_on_executor().value();
		if (partitioned_vertex.get_time_domain()) {
			partitioned_vertices_per_execution_instance[execution_instance_on_executor].push_back(
			    vertex_descriptor);
			m_time_domains[execution_instance_on_executor] =
			    partitioned_vertex.get_time_domain().value();
		}
	}

	std::map<grenade::common::ConnectionOnExecutor, grenade::common::ExecutionInstanceID>
	    next_unused_execution_instance_id;
	for (auto const& vertex_descriptor : get_topology().get_reference().vertices()) {
		auto const& partitioned_vertex = dynamic_cast<grenade::common::PartitionedVertex const&>(
		    get_topology().get_reference().get(vertex_descriptor));
		auto const& execution_instance_on_executor =
		    partitioned_vertex.get_execution_instance_on_executor().value();
		next_unused_execution_instance_id[execution_instance_on_executor.connection_on_executor] =
		    std::max(
		        next_unused_execution_instance_id[execution_instance_on_executor
		                                              .connection_on_executor] +
		            grenade::common::ExecutionInstanceID(1),
		        execution_instance_on_executor.execution_instance_id);
	}

	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::reference_wrapper<ExternalSourceNeuronMapping const>>
	    external_source_neuron_mappings;
	std::map<grenade::common::ExecutionInstanceOnExecutor, grenade::common::VertexOnTopology>
	    crossbar_l2_inputs;

	for (auto const& vertex_descriptor : get_topology().vertices()) {
		if (auto const crossbar_l2_input =
		        dynamic_cast<signal_flow::vertex::CrossbarL2Input const*>(
		            &get_topology().get(vertex_descriptor));
		    crossbar_l2_input) {
			crossbar_l2_inputs.emplace(
			    crossbar_l2_input->get_execution_instance_on_executor().value(), vertex_descriptor);
		}
	}

	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::map<
	        halco::hicann_dls::vx::v3::BackgroundSpikeSourceOnDLS,
	        grenade::common::VertexOnTopology>>
	    background_spike_sources;

	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::map<
	        grenade::common::VertexOnTopology,
	        std::map<halco::hicann_dls::vx::v3::NeuronRowOnDLS, grenade::common::VertexOnTopology>>>
	    locally_placed_neuron_populations;

	std::map<
	    grenade::common::VertexOnTopology,
	    std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>>
	    locally_placed_neuron_anchors;

	// set labels in populations
	std::vector<grenade::common::InterTopologyHyperEdgeOnLinkedTopology> inter_topology_hyper_edges(
	    get_topology().inter_graph_hyper_edges().begin(),
	    get_topology().inter_graph_hyper_edges().end());
	for (auto const& inter_graph_hyper_edge_descriptor : inter_topology_hyper_edges) {
		auto inter_graph_hyper_edge = get_topology().get(inter_graph_hyper_edge_descriptor).copy();
		if (auto* external_source_neuron_mapping =
		        dynamic_cast<ExternalSourceNeuronMapping*>(&(*inter_graph_hyper_edge));
		    external_source_neuron_mapping) {
			// remove partitioned topology populations, which don't require externally-supplied
			// input data.
			// they will get their data from their in edge
			assert(external_source_neuron_mapping->labels.empty());
			for (size_t p = 0; auto const partitioned_vertex_descriptor :
			                   get_topology().references(inter_graph_hyper_edge_descriptor)) {
				external_source_neuron_mapping->labels.push_back({});
				// TODO: could be easier if routing result is stored directly per partitioned
				// vertex, possible since the links are in the graph
				auto const& partitioned_vertex = dynamic_cast<grenade::common::Population const&>(
				    get_topology().get_reference().get(partitioned_vertex_descriptor));
				auto const& partitioned_vertex_labels =
				    routing_result.execution_instances
				        .at(partitioned_vertex.get_execution_instance_on_executor().value())
				        .external_spike_labels.at(partitioned_vertex_descriptor);
				ExternalSourceNeuronMapping::Labels::value_type local_labels;
				for (size_t i = 0; i < partitioned_vertex.get_shape().size(); ++i) {
					external_source_neuron_mapping->labels.at(p)[i] =
					    partitioned_vertex_labels.at(i);
				}
				external_source_neuron_mappings.emplace(
				    partitioned_vertex.get_execution_instance_on_executor().value(),
				    *external_source_neuron_mapping);
				p++;
			}
		} else if (auto* poisson_source_neuron_mapping =
		               dynamic_cast<PoissonSourceNeuronMapping*>(&(*inter_graph_hyper_edge));
		           poisson_source_neuron_mapping) {
			poisson_source_neuron_mapping->labels.resize(
			    get_topology().links(inter_graph_hyper_edge_descriptor).size());
			poisson_source_neuron_mapping->masks.resize(
			    get_topology().links(inter_graph_hyper_edge_descriptor).size());
			for (size_t i = 0; auto const& mapped_vertex_descriptor :
			                   get_topology().links(inter_graph_hyper_edge_descriptor)) {
				for (size_t p = 0; auto const partitioned_vertex_descriptor :
				                   get_topology().references(inter_graph_hyper_edge_descriptor)) {
					// TODO: could be easier if routing result is stored directly per partitioned
					// vertex, possible since the links are in the graph
					auto const& partitioned_vertex =
					    dynamic_cast<grenade::common::Population const&>(
					        get_topology().get_reference().get(partitioned_vertex_descriptor));
					assert(p == 0);
					auto const& local_labels =
					    routing_result.execution_instances
					        .at(partitioned_vertex.get_execution_instance_on_executor().value())
					        .background_spike_source_labels.at(partitioned_vertex_descriptor);

					auto const& mapped_vertex =
					    dynamic_cast<signal_flow::vertex::BackgroundSpikeSource const&>(
					        get_topology().get(mapped_vertex_descriptor));
					background_spike_sources[partitioned_vertex.get_execution_instance_on_executor()
					                             .value()][mapped_vertex.coordinate] =
					    mapped_vertex_descriptor;
					auto const hemisphere = mapped_vertex.coordinate.toPADIBusOnDLS()
					                            .toPADIBusBlockOnDLS()
					                            .toHemisphereOnDLS();
					poisson_source_neuron_mapping->labels.at(i) =
					    local_labels
					        .at(mapped_vertex.coordinate.toPADIBusOnDLS()
					                .toPADIBusBlockOnDLS()
					                .toHemisphereOnDLS())
					        .at(0)
					        .get_neuron_label();
					poisson_source_neuron_mapping->masks.at(i) =
					    routing_result.execution_instances
					        .at(partitioned_vertex.get_execution_instance_on_executor().value())
					        .background_spike_source_masks.at(partitioned_vertex_descriptor)
					        .at(hemisphere);
					p++;
				}
				i++;
			}
		} else if (auto* locally_placed_neuron_mapping =
		               dynamic_cast<LocallyPlacedNeuronMapping*>(&(*inter_graph_hyper_edge));
		           locally_placed_neuron_mapping) {
			auto const partitioned_vertex_descriptor =
			    get_topology().references(inter_graph_hyper_edge_descriptor).at(0);
			auto const& partitioned_vertex = dynamic_cast<grenade::common::Population const&>(
			    get_topology().get_reference().get(partitioned_vertex_descriptor));
			auto const& partitioned_locally_placed_neuron =
			    dynamic_cast<LocallyPlacedNeuron const&>(partitioned_vertex.get_cell());

			locally_placed_neuron_mapping->labels.resize(
			    get_topology().links(inter_graph_hyper_edge_descriptor).size());
			for (size_t i = 0; auto const& mapped_vertex_descriptor :
			                   get_topology().links(inter_graph_hyper_edge_descriptor)) {
				auto const& mapped_vertex = dynamic_cast<signal_flow::vertex::NeuronView const&>(
				    get_topology().get(mapped_vertex_descriptor));
				locally_placed_neuron_populations[mapped_vertex.get_execution_instance_on_executor()
				                                      .value()][partitioned_vertex_descriptor]
				                                 [mapped_vertex.row] = mapped_vertex_descriptor;
				// TODO: could be easier if routing result is stored directly per partitioned
				// vertex, possible since the links are in the graph
				for (auto const& [n, neuron_anchor] : locally_placed_neuron_mapping->anchors) {
					locally_placed_neuron_anchors[partitioned_vertex_descriptor].push_back(
					    neuron_anchor.second);
					auto const placed_compartments =
					    halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
					        partitioned_locally_placed_neuron.shape, neuron_anchor.second)
					        .get_placed_compartments();
					for (auto const& [compartment, atomic_neurons] : placed_compartments) {
						auto const& local_labels =
						    routing_result.execution_instances
						        .at(partitioned_vertex.get_execution_instance_on_executor().value())
						        .internal_neuron_labels.at(partitioned_vertex_descriptor)
						        .at(n)
						        .at(compartment);
						for (size_t an = 0; auto const& atomic_neuron : atomic_neurons) {
							if (atomic_neuron.toNeuronRowOnDLS() == mapped_vertex.row) {
								locally_placed_neuron_mapping->labels.at(i).push_back(
								    local_labels.at(an));
							}
							an++;
						}
					}
				}
				i++;
			}
		}
		get_topology().set(inter_graph_hyper_edge_descriptor, std::move(*inter_graph_hyper_edge));
	}

	std::map<grenade::common::ExecutionInstanceOnExecutor, grenade::common::VertexOnTopology>
	    crossbar_l2_outputs;

	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::map<
	        grenade::common::VertexOnTopology,
	        std::map<halco::hicann_dls::vx::v3::SynramOnDLS, grenade::common::VertexOnTopology>>>
	    projections;

	for (auto const& [execution_instance, partitioned_vertices] :
	     partitioned_vertices_per_execution_instance) {
		auto const& local_routing_result =
		    routing_result.execution_instances.at(execution_instance);

		std::optional<grenade::common::TimeDomainOnTopology> time_domain;

		for (auto const& partitioned_vertex_descriptor : partitioned_vertices) {
			auto const& partitioned_vertex =
			    get_topology().get_reference().get(partitioned_vertex_descriptor);

			if (auto const partitioned_projection =
			        dynamic_cast<grenade::common::Projection const*>(&partitioned_vertex);
			    partitioned_projection) {
				if (auto const partitioned_uncalibrated_synapse =
				        dynamic_cast<UncalibratedSynapse const*>(
				            &partitioned_projection->get_synapse());
				    partitioned_uncalibrated_synapse) {
					auto const& local_connections =
					    local_routing_result.connections.at(partitioned_vertex_descriptor);

					std::vector<
					    std::map<halco::hicann_dls::vx::v3::SynramOnDLS, std::vector<size_t>>>
					    translation(local_connections.size());

					std::map<
					    halco::hicann_dls::vx::v3::SynramOnDLS,
					    std::vector<RoutingResult::ExecutionInstance::PlacedConnection>>
					    placed_connections;
					for (size_t c = 0; auto const& conn : local_connections) {
						for (auto const& syn : conn) {
							translation.at(c)[syn.synapse_row.toSynramOnDLS()].push_back(
							    placed_connections[syn.synapse_row.toSynramOnDLS()].size());
							placed_connections[syn.synapse_row.toSynramOnDLS()].push_back(syn);
						}
						c++;
					}

					std::map<size_t, std::vector<std::vector<size_t>>> mapped_translation;
					UncalibratedSynapseMapping::Labels mapped_labels(placed_connections.size());
					for (size_t c = 0; auto const& lt : translation) {
						mapped_translation[c].resize(placed_connections.size());
						for (auto const& [s, cs] : lt) {
							mapped_translation.at(c).at(std::distance(
							    placed_connections.begin(),
							    std::find_if(
							        placed_connections.begin(), placed_connections.end(),
							        [s](auto const& p) { return p.first == s; }))) = cs;
						}
						c++;
					}
					std::vector<grenade::common::VertexOnTopology> mapped_vertex_descriptors;
					for (auto const& [synram, placed_conns] : placed_connections) {
						size_t ss = std::distance(
						    placed_connections.begin(),
						    std::find_if(
						        placed_connections.begin(), placed_connections.end(),
						        [synram](auto const& p) { return p.first == synram; }));
						std::set<halco::hicann_dls::vx::v3::SynapseRowOnSynram> rows;
						std::set<signal_flow::vertex::SynapseArrayViewSparse::Columns::value_type>
						    columns;
						for (auto const& placed_conn : placed_conns) {
							rows.insert(placed_conn.synapse_row.toSynapseRowOnSynram());
							columns.insert(placed_conn.synapse_on_row);
						}

						signal_flow::vertex::SynapseArrayViewSparse::Synapses synapses;
						for (auto const& placed_conn : placed_conns) {
							synapses.push_back(signal_flow::vertex::SynapseArrayViewSparse::Synapse{
							    static_cast<size_t>(std::distance(
							        rows.begin(),
							        rows.find(placed_conn.synapse_row.toSynapseRowOnSynram()))),
							    static_cast<size_t>(std::distance(
							        columns.begin(), columns.find(placed_conn.synapse_on_row)))});
							mapped_labels.at(ss).push_back(placed_conn.label);
						}
						projections[execution_instance][partitioned_vertex_descriptor][synram] =
						    get_topology().add_vertex(signal_flow::vertex::SynapseArrayViewSparse(
						        synram,
						        signal_flow::vertex::SynapseArrayViewSparse::Rows(
						            rows.begin(), rows.end()),
						        signal_flow::vertex::SynapseArrayViewSparse::Columns(
						            columns.begin(), columns.end()),
						        std::move(synapses), common::ChipOnConnection(),
						        partitioned_vertex.get_time_domain().value(), execution_instance));
						time_domain = partitioned_vertex.get_time_domain().value();
						mapped_vertex_descriptors.push_back(projections.at(execution_instance)
						                                        .at(partitioned_vertex_descriptor)
						                                        .at(synram));

						for (auto const out_edge_descriptor :
						     get_topology().get_reference().out_edges(
						         partitioned_vertex_descriptor)) {
							auto const target_descriptor =
							    get_topology().get_reference().target(out_edge_descriptor);
							if (!locally_placed_neuron_populations.at(execution_instance)
							         .contains(target_descriptor)) {
								continue;
							}
							auto const mapped_target_descriptor =
							    locally_placed_neuron_populations.at(execution_instance)
							        .at(target_descriptor)
							        .at(synram.toNeuronRowOnDLS());
							auto const& mapped_target_vertex =
							    dynamic_cast<signal_flow::vertex::NeuronView const&>(
							        get_topology().get(mapped_target_descriptor));
							auto const& target_columns = mapped_target_vertex.get_columns();
							std::vector<grenade::common::MultiIndex> channels_on_source;
							std::vector<grenade::common::MultiIndex> channels_on_target;
							for (size_t cs = 0; auto const& column : columns) {
								if (auto const it = std::find(
								        target_columns.begin(), target_columns.end(),
								        column.toNeuronColumnOnDLS());
								    it != target_columns.end()) {
									channels_on_source.push_back(grenade::common::MultiIndex({cs}));
									channels_on_target.push_back(
									    grenade::common::MultiIndex({static_cast<size_t>(
									        std::distance(target_columns.begin(), it))}));
								}
								cs++;
							}
							get_topology().add_edge(
							    projections.at(execution_instance)
							        .at(partitioned_vertex_descriptor)
							        .at(synram),
							    mapped_target_descriptor,
							    grenade::common::Edge(
							        grenade::common::ListMultiIndexSequence(
							            std::move(channels_on_source)),
							        grenade::common::ListMultiIndexSequence(
							            std::move(channels_on_target)),
							        0, 0));
						}
						// TODO: add in-edges
					}
					get_topology().add_inter_graph_hyper_edge(
					    std::move(mapped_vertex_descriptors), {partitioned_vertex_descriptor},
					    UncalibratedSynapseMapping(
					        std::move(mapped_translation), std::move(mapped_labels)));
				}
			}
		}

		// add recorders to mapped graph
		std::optional<grenade::common::VertexOnTopology> crossbar_l2_output;
		std::map<
		    grenade::common::VertexOnTopology,
		    std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>>
		    madc_recorder_locations;

		for (auto const& partitioned_vertex_descriptor : partitioned_vertices) {
			auto const& partitioned_vertex =
			    get_topology().get_reference().get(partitioned_vertex_descriptor);

			if (auto const partitioned_spike_recorder =
			        dynamic_cast<SpikeRecorder const*>(&partitioned_vertex);
			    partitioned_spike_recorder) {
				std::vector<signal_flow::SpikeFromChip> local_labels(
				    partitioned_spike_recorder->get_shape().size());
				auto const recorder_shape_elements =
				    partitioned_spike_recorder->get_shape().get_elements();
				for (auto const in_edge_descriptor :
				     get_topology().get_reference().in_edges(partitioned_vertex_descriptor)) {
					auto const source_vertex_descriptor =
					    get_topology().get_reference().source(in_edge_descriptor);
					auto const& in_edge = get_topology().get_reference().get(in_edge_descriptor);
					auto const& partitioned_source_population =
					    dynamic_cast<grenade::common::Population const&>(
					        get_topology().get_reference().get(source_vertex_descriptor));
					auto const target_channels = in_edge.get_channels_on_target().get_elements();
					// match source neurons with routing result labels
					if (auto const* external_source_neuron =
					        dynamic_cast<ExternalSourceNeuron const*>(
					            &partitioned_source_population.get_cell());
					    external_source_neuron) {
						auto const source_channels =
						    grenade::common::CuboidMultiIndexSequence(
						        {partitioned_source_population.size()})
						        .related_sequence_subset_restriction(
						            partitioned_source_population.get_output_ports()
						                .at(in_edge.port_on_source)
						                .get_channels(),
						            in_edge.get_channels_on_source())
						        ->get_elements();
						for (size_t i = 0; i < source_channels.size(); i++) {
							local_labels.at(std::distance(
							    recorder_shape_elements.begin(),
							    std::find(
							        recorder_shape_elements.begin(), recorder_shape_elements.end(),
							        target_channels.at(i)))) =
							    local_routing_result.external_spike_labels
							        .at(source_vertex_descriptor)
							        .at(source_channels.at(i).value.at(0))
							        .at(0);
						}
					} else if (auto const* poisson_source_neuron =
					               dynamic_cast<PoissonSourceNeuron const*>(
					                   &partitioned_source_population.get_cell());
					           poisson_source_neuron) {
						auto const source_channels =
						    grenade::common::CuboidMultiIndexSequence(
						        {partitioned_source_population.size()})
						        .related_sequence_subset_restriction(
						            partitioned_source_population.get_output_ports()
						                .at(in_edge.port_on_source)
						                .get_channels(),
						            in_edge.get_channels_on_source())
						        ->get_elements();
						for (size_t i = 0; i < source_channels.size(); i++) {
							local_labels.at(std::distance(
							    recorder_shape_elements.begin(),
							    std::find(
							        recorder_shape_elements.begin(), recorder_shape_elements.end(),
							        target_channels.at(i)))) =
							    local_routing_result.background_spike_source_labels
							        .at(source_vertex_descriptor)
							        .begin()
							        ->second.at(source_channels.at(i).value.at(0));
						}
					} else if (auto const* internal_source_neuron =
					               dynamic_cast<LocallyPlacedNeuron const*>(
					                   &partitioned_source_population.get_cell());
					           internal_source_neuron) {
						std::set<size_t> cell_on_population_dimensions;
						std::set<size_t> compartment_on_neuron_dimensions;
						for (size_t i = 0; auto const& dimension_unit :
						                   in_edge.get_channels_on_source().get_dimension_units()) {
							if (dimension_unit &&
							    dynamic_cast<grenade::common::CellOnPopulationDimensionUnit const*>(
							        &(*dimension_unit))) {
								cell_on_population_dimensions.insert(i);
							} else {
								compartment_on_neuron_dimensions.insert(i);
							}
							i++;
						}
						auto const neurons_on_population =
						    grenade::common::CuboidMultiIndexSequence(
						        {partitioned_source_population.size()})
						        .related_sequence_subset_restriction(
						            partitioned_source_population.get_shape(),
						            *in_edge.get_channels_on_source().projection(
						                cell_on_population_dimensions))
						        ->get_elements();
						auto const compartments_on_neuron =
						    in_edge.get_channels_on_source()
						        .projection(compartment_on_neuron_dimensions)
						        ->get_elements();
						assert(neurons_on_population.size() == compartments_on_neuron.size());
						for (size_t i = 0; i < neurons_on_population.size(); i++) {
							halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron
							    compartment_on_neuron(compartments_on_neuron.at(i).value.at(0));
							auto const atomic_neuron_on_compartment =
							    internal_source_neuron->compartments
							        .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
							        .spike_master.value()
							        .atomic_neuron_on_compartment;
							auto const atomic_neuron =
							    halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
							        internal_source_neuron->shape,
							        locally_placed_neuron_anchors.at(source_vertex_descriptor)
							            .at(neurons_on_population.at(i).value.at(0)))
							        .get_placed_compartments()
							        .at(compartment_on_neuron)
							        .at(atomic_neuron_on_compartment);

							halco::hicann_dls::vx::v3::SpikeLabel spike_label;
							spike_label.set_neuron_event_output(
							    atomic_neuron.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS());
							spike_label.set_spl1_address(halco::hicann_dls::vx::v3::SPL1Address(
							    atomic_neuron.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS() %
							    halco::hicann_dls::vx::v3::SPL1Address::size));
							spike_label.set_neuron_backend_address_out(
							    local_routing_result.internal_neuron_labels
							        .at(source_vertex_descriptor)
							        .at(neurons_on_population.at(i).value.at(0))
							        .at(compartment_on_neuron)
							        .at(atomic_neuron_on_compartment)
							        .value());
							local_labels.at(std::distance(
							    recorder_shape_elements.begin(),
							    std::find(
							        recorder_shape_elements.begin(), recorder_shape_elements.end(),
							        target_channels.at(i)))) = spike_label;
						}
					}
				}

				time_domain = partitioned_vertex.get_time_domain().value();
				if (!crossbar_l2_output) {
					crossbar_l2_output =
					    get_topology().add_vertex(signal_flow::vertex::CrossbarL2Output(
					        true, common::ChipOnConnection(),
					        partitioned_vertex.get_time_domain().value(), execution_instance));
					crossbar_l2_outputs.emplace(execution_instance, *crossbar_l2_output);
				} else if (
				    get_topology()
				        .get(*crossbar_l2_output)
				        .get_output_ports()
				        .at(0)
				        .requires_or_generates_data ==
				    grenade::common::Vertex::Port::RequiresOrGeneratesData::no) {
					get_topology().set(
					    *crossbar_l2_output,
					    signal_flow::vertex::CrossbarL2Output(
					        true, common::ChipOnConnection(),
					        partitioned_vertex.get_time_domain().value(), execution_instance));
				}

				get_topology().add_inter_graph_hyper_edge(
				    {*crossbar_l2_output}, {partitioned_vertex_descriptor},
				    SpikeRecorderMapping(std::move(local_labels)));
			} else if (auto const partitioned_cadc_recorder =
			               dynamic_cast<CADCRecorder const*>(&partitioned_vertex);
			           partitioned_cadc_recorder) {
				std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> atomic_neurons;
				atomic_neurons.resize(partitioned_cadc_recorder->get_shape().size());
				std::map<
				    grenade::common::VertexOnTopology,
				    std::pair<grenade::common::Edge, grenade::common::EdgeOnTopology>>
				    mapped_edges;
				for (auto const in_edge_descriptor :
				     get_topology().get_reference().in_edges(partitioned_vertex_descriptor)) {
					auto const source_vertex_descriptor =
					    get_topology().get_reference().source(in_edge_descriptor);
					auto const& in_edge = get_topology().get_reference().get(in_edge_descriptor);
					auto const& partitioned_source_population =
					    dynamic_cast<grenade::common::Population const&>(
					        get_topology().get_reference().get(source_vertex_descriptor));
					auto const target_channels_on_recorder =
					    grenade::common::CuboidMultiIndexSequence(
					        {partitioned_cadc_recorder->get_input_ports()
					             .at(in_edge.port_on_target)
					             .get_channels()
					             .size()})
					        .related_sequence_subset_restriction(
					            partitioned_cadc_recorder->get_input_ports()
					                .at(in_edge.port_on_target)
					                .get_channels(),
					            in_edge.get_channels_on_target())
					        ->get_elements();
					auto const& internal_source_neuron = dynamic_cast<LocallyPlacedNeuron const&>(
					    partitioned_source_population.get_cell());

					std::set<size_t> cell_on_population_dimensions;
					std::set<size_t> compartment_on_neuron_dimensions;
					std::set<size_t> atomic_neuron_on_compartment_dimensions;
					for (size_t i = 0; auto const& dimension_unit :
					                   in_edge.get_channels_on_source().get_dimension_units()) {
						if (dimension_unit &&
						    dynamic_cast<grenade::common::CellOnPopulationDimensionUnit const*>(
						        &(*dimension_unit))) {
							cell_on_population_dimensions.insert(i);
						} else if (
						    dimension_unit &&
						    dynamic_cast<grenade::common::CompartmentOnNeuronDimensionUnit const*>(
						        &(*dimension_unit))) {
							compartment_on_neuron_dimensions.insert(i);
						} else {
							atomic_neuron_on_compartment_dimensions.insert(i);
						}
						i++;
					}
					auto const neurons_on_population =
					    grenade::common::CuboidMultiIndexSequence(
					        {partitioned_source_population.size()})
					        .related_sequence_subset_restriction(
					            partitioned_source_population.get_shape(),
					            *in_edge.get_channels_on_source().projection(
					                cell_on_population_dimensions))
					        ->get_elements();
					assert(compartment_on_neuron_dimensions.size() == 1);
					assert(atomic_neuron_on_compartment_dimensions.size() == 1);
					size_t const compartment_on_neuron_dimension =
					    *compartment_on_neuron_dimensions.begin();
					size_t const atomic_neuron_on_compartment_dimension =
					    *atomic_neuron_on_compartment_dimensions.begin();
					auto const compartments_on_neuron =
					    in_edge.get_channels_on_source()
					        .projection(compartment_on_neuron_dimensions)
					        ->get_elements();
					auto const atomic_neurons_on_compartment =
					    in_edge.get_channels_on_source()
					        .projection(atomic_neuron_on_compartment_dimensions)
					        ->get_elements();

					auto const& mapped_neuron_views =
					    locally_placed_neuron_populations.at(execution_instance)
					        .at(source_vertex_descriptor);
					atomic_neurons.resize(neurons_on_population.size());
					for (size_t i = 0; i < neurons_on_population.size(); i++) {
						halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron compartment_on_neuron(
						    in_edge.get_channels_on_source().get_elements().at(i).value.at(
						        compartment_on_neuron_dimension));
						auto const atomic_neuron_on_compartment =
						    in_edge.get_channels_on_source().get_elements().at(i).value.at(
						        atomic_neuron_on_compartment_dimension);
						auto const atomic_neuron =
						    halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
						        internal_source_neuron.shape,
						        locally_placed_neuron_anchors.at(source_vertex_descriptor)
						            .at(neurons_on_population.at(i).value.at(0)))
						        .get_placed_compartments()
						        .at(compartment_on_neuron)
						        .at(atomic_neuron_on_compartment);
						atomic_neurons.at(target_channels_on_recorder.at(i).value.at(0)) =
						    atomic_neuron;

						auto const& mapped_neuron_view_descriptor =
						    mapped_neuron_views.at(atomic_neuron.toNeuronRowOnDLS());
						auto const& mapped_neuron_view =
						    dynamic_cast<signal_flow::vertex::NeuronView const&>(
						        get_topology().get(mapped_neuron_view_descriptor));
						size_t const column = std::distance(
						    mapped_neuron_view.get_columns().begin(),
						    std::find(
						        mapped_neuron_view.get_columns().begin(),
						        mapped_neuron_view.get_columns().end(),
						        atomic_neuron.toNeuronColumnOnDLS()));
						assert(column < mapped_neuron_view.get_columns().size());
						mapped_edges.emplace(
						    mapped_neuron_view_descriptor,
						    std::pair{
						        grenade::common::Edge(
						            grenade::common::ListMultiIndexSequence(
						                {grenade::common::MultiIndex({column})}),
						            grenade::common::ListMultiIndexSequence(
						                {grenade::common::MultiIndex({i})})),
						        in_edge_descriptor});
					}
				}

				std::map<
				    halco::hicann_dls::vx::v3::SynramOnDLS,
				    signal_flow::vertex::CADCMembraneReadoutView::Columns>
				    columns_per_synram;
				for (auto const& an : atomic_neurons) {
					columns_per_synram[an.toNeuronRowOnDLS().toSynramOnDLS()].push_back(
					    an.toNeuronColumnOnDLS().toSynapseOnSynapseRow());
				}

				time_domain = partitioned_vertex.get_time_domain().value();

				std::vector<grenade::common::VertexOnTopology> mapped_vertex_descriptors;
				for (auto const& [synram, columns] : columns_per_synram) {
					mapped_vertex_descriptors.push_back(
					    get_topology().add_vertex(signal_flow::vertex::CADCMembraneReadoutView(
					        columns, synram,
					        partitioned_cadc_recorder->enable_record_to_dram
					            ? signal_flow::vertex::CADCMembraneReadoutView::Mode::
					                  periodic_on_dram
					            : signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic,
					        common::ChipOnConnection(), time_domain.value(), execution_instance)));
				}
				get_topology().add_inter_graph_hyper_edge(
				    mapped_vertex_descriptors, {partitioned_vertex_descriptor},
				    CADCRecorderMapping());

			} else if (auto const partitioned_madc_recorder =
			               dynamic_cast<MADCRecorder const*>(&partitioned_vertex);
			           partitioned_madc_recorder) {
				auto& atomic_neurons = madc_recorder_locations[partitioned_vertex_descriptor];
				atomic_neurons.resize(partitioned_madc_recorder->get_shape().size());
				std::map<
				    grenade::common::VertexOnTopology,
				    std::pair<grenade::common::Edge, grenade::common::EdgeOnTopology>>
				    mapped_edges;
				for (auto const in_edge_descriptor :
				     get_topology().get_reference().in_edges(partitioned_vertex_descriptor)) {
					auto const source_vertex_descriptor =
					    get_topology().get_reference().source(in_edge_descriptor);
					auto const& in_edge = get_topology().get_reference().get(in_edge_descriptor);
					auto const& partitioned_source_population =
					    dynamic_cast<grenade::common::Population const&>(
					        get_topology().get_reference().get(source_vertex_descriptor));
					auto const target_channels_on_recorder =
					    grenade::common::CuboidMultiIndexSequence(
					        {partitioned_madc_recorder->get_input_ports()
					             .at(in_edge.port_on_target)
					             .get_channels()
					             .size()})
					        .related_sequence_subset_restriction(
					            partitioned_madc_recorder->get_input_ports()
					                .at(in_edge.port_on_target)
					                .get_channels(),
					            in_edge.get_channels_on_target())
					        ->get_elements();
					auto const& internal_source_neuron = dynamic_cast<LocallyPlacedNeuron const&>(
					    partitioned_source_population.get_cell());

					std::set<size_t> cell_on_population_dimensions;
					std::set<size_t> compartment_on_neuron_dimensions;
					std::set<size_t> atomic_neuron_on_compartment_dimensions;
					for (size_t i = 0; auto const& dimension_unit :
					                   in_edge.get_channels_on_source().get_dimension_units()) {
						if (dimension_unit &&
						    dynamic_cast<grenade::common::CellOnPopulationDimensionUnit const*>(
						        &(*dimension_unit))) {
							cell_on_population_dimensions.insert(i);
						} else if (
						    dimension_unit &&
						    dynamic_cast<grenade::common::CompartmentOnNeuronDimensionUnit const*>(
						        &(*dimension_unit))) {
							compartment_on_neuron_dimensions.insert(i);
						} else {
							atomic_neuron_on_compartment_dimensions.insert(i);
						}
						i++;
					}
					auto const neurons_on_population =
					    grenade::common::CuboidMultiIndexSequence(
					        {partitioned_source_population.size()})
					        .related_sequence_subset_restriction(
					            partitioned_source_population.get_shape(),
					            *in_edge.get_channels_on_source().projection(
					                cell_on_population_dimensions))
					        ->get_elements();
					assert(compartment_on_neuron_dimensions.size() == 1);
					assert(atomic_neuron_on_compartment_dimensions.size() == 1);
					size_t const compartment_on_neuron_dimension =
					    *compartment_on_neuron_dimensions.begin();
					size_t const atomic_neuron_on_compartment_dimension =
					    *atomic_neuron_on_compartment_dimensions.begin();
					auto const compartments_on_neuron =
					    in_edge.get_channels_on_source()
					        .projection(compartment_on_neuron_dimensions)
					        ->get_elements();
					auto const atomic_neurons_on_compartment =
					    in_edge.get_channels_on_source()
					        .projection(atomic_neuron_on_compartment_dimensions)
					        ->get_elements();

					auto const& mapped_neuron_views =
					    locally_placed_neuron_populations.at(execution_instance)
					        .at(source_vertex_descriptor);
					atomic_neurons.resize(neurons_on_population.size());
					for (size_t i = 0; i < neurons_on_population.size(); i++) {
						halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron compartment_on_neuron(
						    in_edge.get_channels_on_source().get_elements().at(i).value.at(
						        compartment_on_neuron_dimension));
						auto const atomic_neuron_on_compartment =
						    in_edge.get_channels_on_source().get_elements().at(i).value.at(
						        atomic_neuron_on_compartment_dimension);
						auto const atomic_neuron =
						    halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
						        internal_source_neuron.shape,
						        locally_placed_neuron_anchors.at(source_vertex_descriptor)
						            .at(neurons_on_population.at(i).value.at(0)))
						        .get_placed_compartments()
						        .at(compartment_on_neuron)
						        .at(atomic_neuron_on_compartment);
						atomic_neurons.at(target_channels_on_recorder.at(i).value.at(0)) =
						    atomic_neuron;

						auto const& mapped_neuron_view_descriptor =
						    mapped_neuron_views.at(atomic_neuron.toNeuronRowOnDLS());
						auto const& mapped_neuron_view =
						    dynamic_cast<signal_flow::vertex::NeuronView const&>(
						        get_topology().get(mapped_neuron_view_descriptor));
						size_t const column = std::distance(
						    mapped_neuron_view.get_columns().begin(),
						    std::find(
						        mapped_neuron_view.get_columns().begin(),
						        mapped_neuron_view.get_columns().end(),
						        atomic_neuron.toNeuronColumnOnDLS()));
						assert(column < mapped_neuron_view.get_columns().size());
						mapped_edges.emplace(
						    mapped_neuron_view_descriptor,
						    std::pair{
						        grenade::common::Edge(
						            grenade::common::ListMultiIndexSequence(
						                {grenade::common::MultiIndex({column})}),
						            grenade::common::ListMultiIndexSequence(
						                {grenade::common::MultiIndex({i})})),
						        in_edge_descriptor});
					}
				}

				time_domain = partitioned_vertex.get_time_domain().value();
			} else if (auto const partitioned_pad_recorder =
			               dynamic_cast<PadRecorder const*>(&partitioned_vertex);
			           partitioned_pad_recorder) {
				std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> atomic_neurons;
				std::map<
				    grenade::common::VertexOnTopology,
				    std::pair<grenade::common::Edge, grenade::common::EdgeOnTopology>>
				    mapped_edges;
				for (auto const in_edge_descriptor :
				     get_topology().get_reference().in_edges(partitioned_vertex_descriptor)) {
					auto const source_vertex_descriptor =
					    get_topology().get_reference().source(in_edge_descriptor);
					auto const& in_edge = get_topology().get_reference().get(in_edge_descriptor);
					auto const& partitioned_source_population =
					    dynamic_cast<grenade::common::Population const&>(
					        get_topology().get_reference().get(source_vertex_descriptor));
					auto const target_channels = in_edge.get_channels_on_target().get_elements();
					auto const& internal_source_neuron = dynamic_cast<LocallyPlacedNeuron const&>(
					    partitioned_source_population.get_cell());

					std::set<size_t> cell_on_population_dimensions;
					std::set<size_t> compartment_on_neuron_dimensions;
					std::set<size_t> atomic_neuron_on_compartment_dimensions;
					for (size_t i = 0; auto const& dimension_unit :
					                   in_edge.get_channels_on_source().get_dimension_units()) {
						if (dimension_unit &&
						    dynamic_cast<grenade::common::CellOnPopulationDimensionUnit const*>(
						        &(*dimension_unit))) {
							cell_on_population_dimensions.insert(i);
						} else if (
						    dimension_unit &&
						    dynamic_cast<grenade::common::CompartmentOnNeuronDimensionUnit const*>(
						        &(*dimension_unit))) {
							compartment_on_neuron_dimensions.insert(i);
						} else {
							atomic_neuron_on_compartment_dimensions.insert(i);
						}
						i++;
					}
					auto const neurons_on_population =
					    grenade::common::CuboidMultiIndexSequence(
					        {partitioned_source_population.size()})
					        .related_sequence_subset_restriction(
					            partitioned_source_population.get_shape(),
					            *in_edge.get_channels_on_source().projection(
					                cell_on_population_dimensions))
					        ->get_elements();
					auto const compartments_on_neuron =
					    in_edge.get_channels_on_source()
					        .projection(compartment_on_neuron_dimensions)
					        ->get_elements();
					auto const atomic_neurons_on_compartment =
					    in_edge.get_channels_on_source()
					        .projection(atomic_neuron_on_compartment_dimensions)
					        ->get_elements();
					assert(neurons_on_population.size() == compartments_on_neuron.size());
					assert(atomic_neurons_on_compartment.size() == compartments_on_neuron.size());

					auto const& mapped_neuron_views =
					    locally_placed_neuron_populations.at(execution_instance)
					        .at(source_vertex_descriptor);
					for (size_t i = 0; i < neurons_on_population.size(); i++) {
						halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron compartment_on_neuron(
						    compartments_on_neuron.at(i).value.at(0));
						auto const atomic_neuron_on_compartment =
						    atomic_neurons_on_compartment.at(i).value.at(0);
						auto const atomic_neuron =
						    halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
						        internal_source_neuron.shape,
						        locally_placed_neuron_anchors.at(source_vertex_descriptor)
						            .at(neurons_on_population.at(i).value.at(0)))
						        .get_placed_compartments()
						        .at(compartment_on_neuron)
						        .at(atomic_neuron_on_compartment);
						atomic_neurons.push_back(atomic_neuron);

						auto const& mapped_neuron_view_descriptor =
						    mapped_neuron_views.at(atomic_neuron.toNeuronRowOnDLS());
						auto const& mapped_neuron_view =
						    dynamic_cast<signal_flow::vertex::NeuronView const&>(
						        get_topology().get(mapped_neuron_view_descriptor));
						size_t const column = std::distance(
						    mapped_neuron_view.get_columns().begin(),
						    std::find(
						        mapped_neuron_view.get_columns().begin(),
						        mapped_neuron_view.get_columns().end(),
						        atomic_neuron.toNeuronColumnOnDLS()));
						assert(column < mapped_neuron_view.get_columns().size());
						mapped_edges.emplace(
						    mapped_neuron_view_descriptor,
						    std::pair{
						        grenade::common::Edge(
						            grenade::common::ListMultiIndexSequence(
						                {grenade::common::MultiIndex({column})}),
						            grenade::common::ListMultiIndexSequence(
						                {grenade::common::MultiIndex({i})})),
						        in_edge_descriptor});
					}
				}

				assert(atomic_neurons.size() == partitioned_pad_recorder->get_pads().size());
				time_domain = partitioned_vertex.get_time_domain().value();
				std::vector<grenade::common::VertexOnTopology> pad_recorder_vertex_descriptors;
				for (size_t channel_on_recorder = 0;
				     channel_on_recorder < partitioned_pad_recorder->get_pads().size();
				     ++channel_on_recorder) {
					pad_recorder_vertex_descriptors.push_back(
					    get_topology().add_vertex(signal_flow::vertex::PadReadoutView(
					        atomic_neurons.at(channel_on_recorder),
					        partitioned_pad_recorder->get_pads().at(channel_on_recorder),
					        common::ChipOnConnection(),
					        partitioned_vertex.get_time_domain().value(), execution_instance)));
				}
				get_topology().add_inter_graph_hyper_edge(
				    pad_recorder_vertex_descriptors, {partitioned_vertex_descriptor},
				    PadRecorderMapping());
			}
		}

		// add MADC recorder
		if (!madc_recorder_locations.empty()) {
			std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> atomic_neurons;
			std::vector<grenade::common::VertexOnTopology> partitioned_vertex_descriptors;
			for (auto const& [partitioned_vertex_descriptor, ans] : madc_recorder_locations) {
				atomic_neurons.insert(atomic_neurons.end(), ans.begin(), ans.end());
				partitioned_vertex_descriptors.push_back(partitioned_vertex_descriptor);
			}
			assert(atomic_neurons.size() <= 2);
			grenade::common::VertexOnTopology mapped_vertex_descriptor;
			if (atomic_neurons.size() == 1) {
				mapped_vertex_descriptor =
				    get_topology().add_vertex(signal_flow::vertex::MADCReadoutView(
				        atomic_neurons.at(0), std::nullopt, common::ChipOnConnection(),
				        time_domain.value(), execution_instance));
			} else {
				mapped_vertex_descriptor =
				    get_topology().add_vertex(signal_flow::vertex::MADCReadoutView(
				        atomic_neurons.at(0), atomic_neurons.at(1), common::ChipOnConnection(),
				        time_domain.value(), execution_instance));
			}

			get_topology().add_inter_graph_hyper_edge(
			    {mapped_vertex_descriptor}, std::move(partitioned_vertex_descriptors),
			    MADCRecorderMapping());
		}
	}

	for (auto const& [execution_instance, partitioned_vertices] :
	     partitioned_vertices_per_execution_instance) {
		auto const& local_routing_result =
		    routing_result.execution_instances.at(execution_instance);

		// add inter-exection-instance spike forwarding
		for (auto const& partitioned_vertex_descriptor : partitioned_vertices) {
			auto const& partitioned_vertex =
			    get_topology().get_reference().get(partitioned_vertex_descriptor);

			if (auto const population =
			        dynamic_cast<grenade::common::Population const*>(&partitioned_vertex);
			    population) {
				if (auto const external_source_neuron =
				        dynamic_cast<ExternalSourceNeuron const*>(&population->get_cell());
				    external_source_neuron) {
					if (get_topology().get_reference().in_degree(partitioned_vertex_descriptor) ==
					    0) {
						continue;
					}
					for (auto const& in_edge_descriptor :
					     get_topology().get_reference().in_edges(partitioned_vertex_descriptor)) {
						auto const source_descriptor =
						    get_topology().get_reference().source(in_edge_descriptor);
						auto const& source = get_topology().get_reference().get(source_descriptor);

						auto const& in_edge =
						    get_topology().get_reference().get(in_edge_descriptor);
						auto const channels_on_external =
						    grenade::common::CuboidMultiIndexSequence(
						        {population->get_input_ports().at(0).get_channels().size()})
						        .related_sequence_subset_restriction(
						            population->get_input_ports().at(0).get_channels(),
						            in_edge.get_channels_on_target())
						        ->get_elements();

						if (auto const source_population =
						        dynamic_cast<grenade::common::Population const*>(&source);
						    source_population) {
							if (auto const delay_cell =
							        dynamic_cast<DelayCell const*>(&source_population->get_cell());
							    delay_cell) {
								for (auto const& delay_in_edge_descriptor :
								     get_topology().get_reference().in_edges(source_descriptor)) {
									auto const& delay_source_descriptor =
									    get_topology().get_reference().source(
									        delay_in_edge_descriptor);
									if (auto const spike_recorder =
									        dynamic_cast<SpikeRecorder const*>(
									            &get_topology().get_reference().get(
									                delay_source_descriptor));
									    spike_recorder) {
										auto const execution_instance =
										    spike_recorder->get_execution_instance_on_executor()
										        .value();
										auto const& crossbar_l2_output =
										    crossbar_l2_outputs.at(execution_instance);
										std::optional<
										    std::reference_wrapper<SpikeRecorderMapping const>>
										    spike_recorder_mapping;
										for (auto const inter_topology_hyper_edge :
										     get_topology().inter_graph_hyper_edges_by_reference(
										         delay_source_descriptor)) {
											spike_recorder_mapping =
											    dynamic_cast<SpikeRecorderMapping const&>(
											        get_topology().get(inter_topology_hyper_edge));
										}
										assert(spike_recorder_mapping);

										auto const output_channels_on_delay =
										    grenade::common::CuboidMultiIndexSequence(
										        {source_population->get_shape().size()})
										        .related_sequence_subset_restriction(
										            source_population->get_output_ports()
										                .at(0)
										                .get_channels(),
										            in_edge.get_channels_on_source())
										        ->get_elements();

										auto const& delay_in_edge =
										    get_topology().get_reference().get(
										        delay_in_edge_descriptor);
										auto const channels_on_spike_recorder =
										    grenade::common::CuboidMultiIndexSequence(
										        {spike_recorder->get_shape().size()})
										        .related_sequence_subset_restriction(
										            spike_recorder->get_shape(),
										            delay_in_edge.get_channels_on_source())
										        ->get_elements();
										auto const channels_on_delay =
										    grenade::common::CuboidMultiIndexSequence(
										        {source_population->get_input_ports()
										             .at(0)
										             .get_channels()
										             .size()})
										        .related_sequence_subset_restriction(
										            source_population->get_input_ports()
										                .at(0)
										                .get_channels(),
										            delay_in_edge.get_channels_on_target())
										        ->get_elements();
										DelayCellMapping::Labels delay_cell_mapping_labels;
										for (size_t c = 0; c < channels_on_spike_recorder.size();
										     ++c) {
											auto const& channel_on_spike_recorder =
											    channels_on_spike_recorder.at(c);
											auto const& channel_on_delay = channels_on_delay.at(c);

											auto output_channel_on_delay_it = std::find(
											    output_channels_on_delay.begin(),
											    output_channels_on_delay.end(), channel_on_delay);
											if (output_channel_on_delay_it ==
											    output_channels_on_delay.end()) {
												continue;
											}
											size_t const external_population_index = std::distance(
											    output_channels_on_delay.begin(),
											    output_channel_on_delay_it);
											auto const& channel_on_population =
											    channels_on_external.at(external_population_index);

											auto const& spike_label =
											    spike_recorder_mapping->get().labels.at(
											        channel_on_spike_recorder.value.at(0));

											auto const& local_labels =
											    routing_result.execution_instances
											        .at(population
											                ->get_execution_instance_on_executor()
											                .value())
											        .external_spike_labels.at(
											            partitioned_vertex_descriptor);

											delay_cell_mapping_labels[channel_on_delay.value.at(
											    0)] = std::pair{
											    spike_label,
											    local_labels.at(channel_on_population.value.at(0))};
										}

										auto const spike_lookup_vertex_descriptor =
										    get_topology().add_vertex(
										        signal_flow::vertex::Transformation(
										            network::vertex::transformation::SpikeLookup(),
										            source_population
										                ->get_execution_instance_on_executor()));

										get_topology().add_inter_graph_hyper_edge(
										    {spike_lookup_vertex_descriptor}, {source_descriptor},
										    DelayCellMapping(std::move(delay_cell_mapping_labels)));

										get_topology().add_edge(
										    crossbar_l2_output, spike_lookup_vertex_descriptor,
										    grenade::common::Edge(
										        grenade::common::CuboidMultiIndexSequence({1}),
										        grenade::common::CuboidMultiIndexSequence({1})));

										get_topology().add_edge(
										    spike_lookup_vertex_descriptor,
										    crossbar_l2_inputs.at(
										        population->get_execution_instance_on_executor()
										            .value()),
										    grenade::common::Edge(
										        grenade::common::CuboidMultiIndexSequence({1}),
										        grenade::common::CuboidMultiIndexSequence({1})));
									} else {
										throw std::runtime_error(
										    "Delay cell population getting input from vertex other "
										    "than spike recorder is not supported.");
									}
								}
							} else {
								throw std::runtime_error(
								    "External source neuron population getting input from other "
								    "population which is not a delay cell population is not "
								    "supported.");
							}
						} else if (auto const spike_recorder =
						               dynamic_cast<SpikeRecorder const*>(&source);
						           spike_recorder) {
							auto const execution_instance =
							    spike_recorder->get_execution_instance_on_executor().value();
							auto const& crossbar_l2_output =
							    crossbar_l2_outputs.at(execution_instance);
							std::optional<std::reference_wrapper<SpikeRecorderMapping const>>
							    spike_recorder_mapping;
							for (auto const inter_topology_hyper_edge :
							     get_topology().inter_graph_hyper_edges_by_reference(
							         source_descriptor)) {
								spike_recorder_mapping = dynamic_cast<SpikeRecorderMapping const&>(
								    get_topology().get(inter_topology_hyper_edge));
							}
							assert(spike_recorder_mapping);

							auto const channels_on_spike_recorder =
							    grenade::common::CuboidMultiIndexSequence(
							        {population->get_shape().size()})
							        .related_sequence_subset_restriction(
							            population->get_output_ports().at(0).get_channels(),
							            in_edge.get_channels_on_source())
							        ->get_elements();

							network::vertex::transformation::SpikeLookup::Parameterization::
							    Translation translation;
							for (size_t c = 0; c < channels_on_spike_recorder.size(); ++c) {
								auto const& channel_on_spike_recorder =
								    channels_on_spike_recorder.at(c);
								auto const& channel_on_population = channels_on_external.at(c);

								auto const& spike_label = spike_recorder_mapping->get().labels.at(
								    channel_on_spike_recorder.value.at(0));

								auto const& local_labels =
								    routing_result.execution_instances
								        .at(population->get_execution_instance_on_executor()
								                .value())
								        .external_spike_labels.at(partitioned_vertex_descriptor)
								        .at(channel_on_population.value.at(0));

								for (auto const& local_label : local_labels) {
									translation[spike_label].push_back(
									    {local_label, common::Time()});
								}
							}

							// place on new execution instance
							auto const spike_lookup_vertex_descriptor =
							    get_topology().add_vertex(signal_flow::vertex::Transformation(
							        network::vertex::transformation::SpikeLookup(),
							        grenade::common::ExecutionInstanceOnExecutor(
							            next_unused_execution_instance_id.at(
							                execution_instance.connection_on_executor),
							            execution_instance.connection_on_executor)));
							next_unused_execution_instance_id.at(
							    execution_instance.connection_on_executor) +=
							    grenade::common::ExecutionInstanceID(1);

							get_topology().add_inter_graph_hyper_edge(
							    {spike_lookup_vertex_descriptor}, {},
							    grenade::common::FixtureInterTopologyHyperEdge(
							        1,
							        network::vertex::transformation::SpikeLookup::Parameterization(
							            std::move(translation))));

							get_topology().add_edge(
							    crossbar_l2_output, spike_lookup_vertex_descriptor,
							    grenade::common::Edge(
							        grenade::common::CuboidMultiIndexSequence({1}),
							        grenade::common::CuboidMultiIndexSequence({1})));

							get_topology().add_edge(
							    spike_lookup_vertex_descriptor,
							    crossbar_l2_inputs.at(
							        population->get_execution_instance_on_executor().value()),
							    grenade::common::Edge(
							        grenade::common::CuboidMultiIndexSequence({1}),
							        grenade::common::CuboidMultiIndexSequence({1})));
						} else {
							throw std::runtime_error(
							    "External source neuron population getting input from other "
							    "population which is not a delay cell population  or vertex which "
							    "is not a spike recorder is not "
							    "supported.");
						}
					}
				}
			}
		}

		// add and connect synapse drivers
		std::map<halco::hicann_dls::vx::v3::SynapseDriverOnDLS, grenade::common::VertexOnTopology>
		    synapse_driver_vertex_descriptors;
		for (auto const& [synapse_driver_on_dls, compare_mask] :
		     local_routing_result.synapse_driver_compare_masks) {
			synapse_driver_vertex_descriptors[synapse_driver_on_dls] =
			    get_topology().add_vertex(signal_flow::vertex::SynapseDriver(
			        synapse_driver_on_dls, common::ChipOnConnection(),
			        m_time_domains.at(execution_instance), execution_instance));
			signal_flow::vertex::SynapseDriver::Parameterization parameterization;
			parameterization.enable_address_out = true;
			parameterization.row_address_compare_mask = compare_mask;
			for (auto const& synapse_row : synapse_driver_on_dls.toSynapseRowOnDLS()) {
				parameterization.row_modes[synapse_row.toSynapseRowOnSynapseDriver()] =
				    local_routing_result.synapse_row_modes.at(synapse_row);
			}
			get_topology().add_inter_graph_hyper_edge(
			    {synapse_driver_vertex_descriptors.at(synapse_driver_on_dls)}, {},
			    grenade::common::FixtureInterTopologyHyperEdge(1, std::move(parameterization)));

			for (auto const& [partitioned_vertex_descriptor, synapses] :
			     projections.at(execution_instance)) {
				for (auto const& [synram, synapse_vertex_descriptor] : synapses) {
					if (synram !=
					    synapse_driver_on_dls.toSynapseDriverBlockOnDLS().toSynramOnDLS()) {
						continue;
					}

					auto const& synapse_vertex =
					    dynamic_cast<signal_flow::vertex::SynapseArrayViewSparse const&>(
					        get_topology().get(synapse_vertex_descriptor));

					std::vector<grenade::common::MultiIndex> channels_on_target;
					std::vector<grenade::common::MultiIndex> channels_on_source;
					for (auto const& synapse_row : synapse_driver_on_dls.toSynapseRowOnDLS()) {
						if (auto const it = std::find(
						        synapse_vertex.get_rows().begin(), synapse_vertex.get_rows().end(),
						        synapse_row.toSynapseRowOnSynram());
						    it != synapse_vertex.get_rows().end()) {
							channels_on_target.push_back(
							    grenade::common::MultiIndex({static_cast<size_t>(
							        std::distance(synapse_vertex.get_rows().begin(), it))}));
							channels_on_source.push_back(grenade::common::MultiIndex(
							    {synapse_row.toSynapseRowOnSynapseDriver().value()}));
						}
					}
					if (channels_on_target.empty()) {
						continue;
					}
					get_topology().add_edge(
					    synapse_driver_vertex_descriptors.at(synapse_driver_on_dls),
					    synapse_vertex_descriptor,
					    grenade::common::Edge(
					        grenade::common::ListMultiIndexSequence(std::move(channels_on_source)),
					        grenade::common::ListMultiIndexSequence(
					            std::move(channels_on_target))));
				}
			}
		}

		// add and connect padi-busses
		std::map<halco::hicann_dls::vx::v3::PADIBusOnDLS, grenade::common::VertexOnTopology>
		    padi_bus_vertex_descriptors;
		for (auto const padi_bus_on_dls :
		     halco::common::iter_all<halco::hicann_dls::vx::v3::PADIBusOnDLS>()) {
			if (std::none_of(
			        synapse_driver_vertex_descriptors.begin(),
			        synapse_driver_vertex_descriptors.end(), [padi_bus_on_dls](auto const& s) {
				        return padi_bus_on_dls ==
				               halco::hicann_dls::vx::v3::PADIBusOnDLS(
				                   s.first.toSynapseDriverOnSynapseDriverBlock()
				                       .toPADIBusOnPADIBusBlock(),
				                   s.first.toSynapseDriverBlockOnDLS().toPADIBusBlockOnDLS());
			        })) {
				continue;
			}
			padi_bus_vertex_descriptors[padi_bus_on_dls] =
			    get_topology().add_vertex(signal_flow::vertex::PADIBus(
			        padi_bus_on_dls, common::ChipOnConnection(),
			        m_time_domains.at(execution_instance), execution_instance));

			for (auto const& [synapse_driver_on_dls, synapse_driver_vertex_descriptor] :
			     synapse_driver_vertex_descriptors) {
				if (padi_bus_on_dls ==
				    halco::hicann_dls::vx::v3::PADIBusOnDLS(
				        synapse_driver_on_dls.toSynapseDriverOnSynapseDriverBlock()
				            .toPADIBusOnPADIBusBlock(),
				        synapse_driver_on_dls.toSynapseDriverBlockOnDLS().toPADIBusBlockOnDLS())) {
					get_topology().add_edge(
					    padi_bus_vertex_descriptors.at(padi_bus_on_dls),
					    synapse_driver_vertex_descriptor,
					    grenade::common::Edge(
					        grenade::common::CuboidMultiIndexSequence({1}),
					        grenade::common::CuboidMultiIndexSequence({1})));
				}
			}
		}

		// add and connect crossbar nodes
		std::map<halco::hicann_dls::vx::v3::PADIBusOnDLS, grenade::common::VertexOnTopology>
		    crossbar_node_vertex_descriptors;
		for (auto const& [crossbar_node_on_dls, crossbar_node_config] :
		     local_routing_result.crossbar_nodes) {
			auto const crossbar_node_vertex_descriptor =
			    get_topology().add_vertex(signal_flow::vertex::CrossbarNode(
			        crossbar_node_on_dls, common::ChipOnConnection(),
			        m_time_domains.at(execution_instance), execution_instance));
			get_topology().add_inter_graph_hyper_edge(
			    {crossbar_node_vertex_descriptor}, {},
			    grenade::common::FixtureInterTopologyHyperEdge(
			        1, signal_flow::vertex::CrossbarNode::Parameterization(crossbar_node_config)));

			if (crossbar_node_on_dls.toCrossbarInputOnDLS().toSPL1Address()) {
				if (crossbar_l2_inputs.contains(execution_instance)) {
					get_topology().add_edge(
					    crossbar_l2_inputs.at(execution_instance), crossbar_node_vertex_descriptor,
					    grenade::common::Edge(
					        grenade::common::CuboidMultiIndexSequence({1}),
					        grenade::common::CuboidMultiIndexSequence({1})));
				}
			}

			if (crossbar_node_on_dls.toCrossbarInputOnDLS().toNeuronEventOutputOnDLS()) {
				if (locally_placed_neuron_populations.contains(execution_instance)) {
					for (auto const& [_, neurons] :
					     locally_placed_neuron_populations.at(execution_instance)) {
						for (auto const& [neuron_row_on_dls, neuron_vertex_descriptor] : neurons) {
							auto const& neuron_vertex =
							    dynamic_cast<signal_flow::vertex::NeuronView const&>(
							        get_topology().get(neuron_vertex_descriptor));
							std::vector<grenade::common::MultiIndex> columns_on_neuron_event_output;
							for (size_t c = 0; auto const& column : neuron_vertex.get_columns()) {
								if (column.toNeuronEventOutputOnDLS() ==
								    *crossbar_node_on_dls.toCrossbarInputOnDLS()
								         .toNeuronEventOutputOnDLS()) {
									columns_on_neuron_event_output.push_back(
									    grenade::common::MultiIndex({c}));
								}
								c++;
							}
							if (columns_on_neuron_event_output.empty()) {
								continue;
							}
							get_topology().add_edge(
							    neuron_vertex_descriptor, crossbar_node_vertex_descriptor,
							    grenade::common::Edge(
							        grenade::common::ListMultiIndexSequence(
							            std::move(columns_on_neuron_event_output)),
							        grenade::common::ListMultiIndexSequence(
							            std::vector<grenade::common::MultiIndex>(
							                columns_on_neuron_event_output.size(),
							                grenade::common::MultiIndex({0})))));
						}
					}
				}
			}

			if (background_spike_sources.contains(execution_instance)) {
				for (auto const& [background_spike_source_on_dls, background_vertex_descriptor] :
				     background_spike_sources.at(execution_instance)) {
					if (background_spike_source_on_dls.toCrossbarInputOnDLS() ==
					    crossbar_node_on_dls.toCrossbarInputOnDLS()) {
						get_topology().add_edge(
						    background_vertex_descriptor, crossbar_node_vertex_descriptor,
						    grenade::common::Edge(
						        grenade::common::CuboidMultiIndexSequence({1}),
						        grenade::common::CuboidMultiIndexSequence({1})));
					}
				}
			}

			if (crossbar_node_on_dls.toCrossbarOutputOnDLS().toCrossbarL2OutputOnDLS()) {
				if (crossbar_l2_outputs.contains(execution_instance)) {
					get_topology().add_edge(
					    crossbar_node_vertex_descriptor, crossbar_l2_outputs.at(execution_instance),
					    grenade::common::Edge(
					        grenade::common::CuboidMultiIndexSequence({1}),
					        grenade::common::CuboidMultiIndexSequence({1})));
				}
			}

			if (crossbar_node_on_dls.toCrossbarOutputOnDLS().toPADIBusOnDLS()) {
				if (padi_bus_vertex_descriptors.contains(
				        *crossbar_node_on_dls.toCrossbarOutputOnDLS().toPADIBusOnDLS())) {
					get_topology().add_edge(
					    crossbar_node_vertex_descriptor,
					    padi_bus_vertex_descriptors.at(
					        *crossbar_node_on_dls.toCrossbarOutputOnDLS().toPADIBusOnDLS()),
					    grenade::common::Edge(
					        grenade::common::CuboidMultiIndexSequence({1}),
					        grenade::common::CuboidMultiIndexSequence({1})));
				}
			}
		}

		for (auto const& partitioned_vertex_descriptor : partitioned_vertices) {
			if (auto const partitioned_plasticity =
			        dynamic_cast<grenade::vx::network::abstract::PlasticityRule const*>(
			            &get_topology().get_reference().get(partitioned_vertex_descriptor));
			    partitioned_plasticity) {
				std::map<size_t, std::vector<signal_flow::vertex::PlasticityRule::SynapseViewShape>>
				    synapse_view_shapes_map;
				std::map<size_t, std::vector<signal_flow::vertex::PlasticityRule::NeuronViewShape>>
				    neuron_view_shapes_map;
				std::map<size_t, std::vector<grenade::common::VertexOnTopology>>
				    synapse_view_vertices;
				std::map<size_t, std::vector<grenade::common::VertexOnTopology>>
				    neuron_view_vertices;
				for (auto const& in_edge :
				     get_topology().get_reference().in_edges(partitioned_vertex_descriptor)) {
					auto const source_vertex_descriptor =
					    get_topology().get_reference().source(in_edge);
					auto const& source_vertex =
					    dynamic_cast<grenade::common::PartitionedVertex const&>(
					        get_topology().get_reference().get(source_vertex_descriptor));
					if (auto const source_population_section =
					        dynamic_cast<grenade::common::Population const*>(&source_vertex);
					    source_population_section) {
						for (auto const& [neuron_row_on_dls, neuron_vertex_descriptor] :
						     locally_placed_neuron_populations.at(execution_instance)
						         .at(source_vertex_descriptor)) {
							auto const& neuron_vertex =
							    dynamic_cast<signal_flow::vertex::NeuronView const&>(
							        get_topology().get(neuron_vertex_descriptor));
							neuron_view_shapes_map
							    [get_topology().get_reference().get(in_edge).port_on_target -
							     partitioned_plasticity->get_projection_shapes().size()]
							        .push_back(signal_flow::vertex::PlasticityRule::NeuronViewShape{

							            neuron_vertex.get_columns(), neuron_row_on_dls});
							neuron_view_vertices
							    [get_topology().get_reference().get(in_edge).port_on_target -
							     partitioned_plasticity->get_projection_shapes().size()]
							        .push_back(neuron_vertex_descriptor);
						}
					}
					if (auto const source_projection_section =
					        dynamic_cast<grenade::common::Projection const*>(&source_vertex);
					    source_projection_section) {
						for (auto const& [synram_on_dls, synapse_vertex_descriptor] :
						     projections.at(execution_instance).at(source_vertex_descriptor)) {
							auto const& synapse_vertex =
							    dynamic_cast<signal_flow::vertex::SynapseArrayViewSparse const&>(
							        get_topology().get(synapse_vertex_descriptor));
							synapse_view_shapes_map
							    [get_topology().get_reference().get(in_edge).port_on_target]
							        .push_back(
							            signal_flow::vertex::PlasticityRule::SynapseViewShape{
							                synapse_vertex.get_rows().size(),
							                {synapse_vertex.get_columns().begin(),
							                 synapse_vertex.get_columns().end()},
							                synram_on_dls.toHemisphereOnDLS()});
							synapse_view_vertices
							    [get_topology().get_reference().get(in_edge).port_on_target]
							        .push_back(synapse_vertex_descriptor);
						}
					}
				}
				std::vector<signal_flow::vertex::PlasticityRule::SynapseViewShape>
				    synapse_view_shapes;
				std::vector<signal_flow::vertex::PlasticityRule::NeuronViewShape>
				    neuron_view_shapes;
				std::map<grenade::common::VertexOnTopology, size_t> synapse_view_vertices_reverse;
				std::map<grenade::common::VertexOnTopology, size_t> neuron_view_vertices_reverse;
				size_t i_synapse = 0;
				for (auto const& [synapse_view_index, local_synapse_view_shapes] :
				     synapse_view_shapes_map) {
					for (size_t i = 0;
					     auto const& local_synapse_view_shape : local_synapse_view_shapes) {
						synapse_view_shapes.push_back(local_synapse_view_shape);
						synapse_view_vertices_reverse[synapse_view_vertices[synapse_view_index].at(
						    i)] = i_synapse;
						i_synapse++;
						i++;
					}
				}
				size_t i_neuron = 0;
				for (auto const& [neuron_view_index, local_neuron_view_shapes] :
				     neuron_view_shapes_map) {
					for (size_t i = 0;
					     auto const& local_neuron_view_shape : local_neuron_view_shapes) {
						neuron_view_shapes.push_back(local_neuron_view_shape);
						neuron_view_vertices_reverse[neuron_view_vertices[neuron_view_index].at(
						    i)] = i_neuron;
						i_neuron++;
						i++;
					}
				}
				size_t const num_synapses = synapse_view_shapes.size();
				auto const mapped_vertex_descriptor =
				    get_topology().add_vertex(signal_flow::vertex::PlasticityRule(
				        std::move(synapse_view_shapes), std::move(neuron_view_shapes),
				        partitioned_plasticity->recording, partitioned_plasticity->id,
				        common::ChipOnConnection(), m_time_domains.at(execution_instance),
				        execution_instance));
				get_topology().add_inter_graph_hyper_edge(
				    {mapped_vertex_descriptor}, {partitioned_vertex_descriptor},
				    PlasticityRuleMapping());
				// add in-edges
				for (auto const& in_edge :
				     get_topology().get_reference().in_edges(partitioned_vertex_descriptor)) {
					auto const source_vertex_descriptor =
					    get_topology().get_reference().source(in_edge);
					auto const& source_vertex =
					    dynamic_cast<grenade::common::PartitionedVertex const&>(
					        get_topology().get_reference().get(source_vertex_descriptor));
					if (auto const source_population_section =
					        dynamic_cast<grenade::common::Population const*>(&source_vertex);
					    source_population_section) {
						for (auto const& [neuron_row_on_dls, neuron_vertex_descriptor] :
						     locally_placed_neuron_populations.at(execution_instance)
						         .at(source_vertex_descriptor)) {
							auto const& neuron_vertex =
							    dynamic_cast<signal_flow::vertex::NeuronView const&>(
							        get_topology().get(neuron_vertex_descriptor));
							get_topology().add_edge(
							    neuron_vertex_descriptor, mapped_vertex_descriptor,
							    grenade::common::Edge(
							        grenade::common::CuboidMultiIndexSequence(
							            {neuron_vertex.get_columns().size()}),
							        grenade::common::CuboidMultiIndexSequence(
							            {neuron_vertex.get_columns().size()}),
							        0,
							        num_synapses +
							            neuron_view_vertices_reverse.at(neuron_vertex_descriptor)));
						}
					}
					if (auto const source_projection_section =
					        dynamic_cast<grenade::common::Projection const*>(&source_vertex);
					    source_projection_section) {
						for (auto const& [synram_on_dls, synapse_vertex_descriptor] :
						     projections.at(execution_instance).at(source_vertex_descriptor)) {
							auto const& synapse_vertex =
							    dynamic_cast<signal_flow::vertex::SynapseArrayViewSparse const&>(
							        get_topology().get(synapse_vertex_descriptor));
							get_topology().add_edge(
							    synapse_vertex_descriptor, mapped_vertex_descriptor,
							    grenade::common::Edge(
							        grenade::common::CuboidMultiIndexSequence(
							            {synapse_vertex.get_columns().size()}),
							        grenade::common::CuboidMultiIndexSequence(
							            {synapse_vertex.get_columns().size()}),
							        0,
							        synapse_view_vertices_reverse.at(synapse_vertex_descriptor)));
						}
					}
				}
			}
		}
	}

	for (auto const& [_, time_domain] : m_time_domains) {
		get_topology().inter_topology_time_domain_edges.emplace(time_domain, time_domain);
	}
}

} // namespace grenade::vx::network::abstract
