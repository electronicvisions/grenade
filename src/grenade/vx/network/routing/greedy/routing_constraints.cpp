#include "grenade/vx/network/routing/greedy/routing_constraints.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence/cartesian_product.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/multi_index_sequence_dimension_unit/receptor_on_compartment.h"
#include "grenade/common/population.h"
#include "grenade/common/projection.h"
#include "grenade/common/projection_connector/static.h"
#include "grenade/common/receptor_on_compartment.h"
#include "grenade/common/recorder.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/mapping/locally_placed_neuron.h"
#include "grenade/vx/network/abstract/mapping/poisson_source_neuron.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/network/abstract/population_cell/poisson_source.h"
#include "grenade/vx/signal_flow/vertex/background_spike_source.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/math.h"

namespace grenade::vx::network::routing::greedy {

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;

RoutingConstraints::RoutingConstraints(
    grenade::common::LinkedTopology const& topology,
    std::vector<grenade::common::VertexOnTopology> const& partitioned_vertex_descriptors,
    ConnectionRoutingResult const& connection_routing_result) :
    m_topology(topology),
    m_partitioned_vertex_descriptors(partitioned_vertex_descriptors),
    m_connection_routing_result(connection_routing_result)
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
	if (!m_internal_connections.empty()) {
		return m_internal_connections;
	}
	auto const anchors = get_anchors();
	std::vector<InternalConnection> ret;
	for (auto const& partitioned_vertex_descriptor : m_partitioned_vertex_descriptors) {
		if (auto const projection = dynamic_cast<grenade::common::Projection const*>(
		        &m_topology.get_reference().get(partitioned_vertex_descriptor))) {
			if (auto const static_connector = dynamic_cast<grenade::common::StaticConnector const*>(
			        &projection->get_connector());
			    static_connector) {
				auto const section =
				    projection->get_connector().get_input_sequence()->cartesian_product(
				        *projection->get_connector().get_output_sequence());
				auto const synapse_sequence = static_connector->get_synapse_connections(*section);

				for (auto const in_edge_descriptor :
				     m_topology.get_reference().in_edges(partitioned_vertex_descriptor)) {
					auto const source_vertex_descriptor =
					    m_topology.get_reference().source(in_edge_descriptor);
					if (std::find(
					        m_partitioned_vertex_descriptors.begin(),
					        m_partitioned_vertex_descriptors.end(),
					        source_vertex_descriptor) == m_partitioned_vertex_descriptors.end()) {
						// source not on same execution instance
						continue;
					}
					auto const& population_pre = dynamic_cast<grenade::common::Population const&>(
					    m_topology.get_reference().get(
					        m_topology.get_reference().source(in_edge_descriptor)));
					auto const neuron_pre = dynamic_cast<abstract::LocallyPlacedNeuron const*>(
					    &population_pre.get_cell());
					if (!neuron_pre) {
						continue;
					}
					auto const& in_edge = m_topology.get_reference().get(in_edge_descriptor);
					auto const population_pre_shape_elements =
					    population_pre.get_shape().get_elements();

					std::set<size_t> synapse_input_dimensions;
					for (size_t i = 0;
					     i < projection->get_connector().get_input_sequence()->dimensionality();
					     ++i) {
						synapse_input_dimensions.insert(i);
					}

					std::set<size_t> neuron_pre_neuron_dimensions;
					std::set<size_t> neuron_pre_compartment_dimensions;
					auto const& neuron_pre_ports = population_pre.get_output_ports();
					auto const& neuron_pre_channels =
					    neuron_pre_ports.at(in_edge.port_on_source).get_channels();
					for (size_t i = 0; i < neuron_pre_channels.dimensionality(); ++i) {
						if (dynamic_cast<grenade::common::CellOnPopulationDimensionUnit const*>(
						        &(*neuron_pre_channels.get_dimension_units().at(i)))) {
							neuron_pre_neuron_dimensions.insert(i);
						} else if (dynamic_cast<
						               grenade::common::CompartmentOnNeuronDimensionUnit const*>(
						               &(*neuron_pre_channels.get_dimension_units().at(i)))) {
							neuron_pre_compartment_dimensions.insert(i);
						}
					}
					assert(neuron_pre_compartment_dimensions.size() == 1);
					size_t const neuron_pre_compartment_dimension =
					    *neuron_pre_compartment_dimensions.begin();
					auto const local_in_edge_channels_on_source_elements =
					    in_edge.get_channels_on_source().get_elements();
					auto const local_in_edge_channels_on_target_elements =
					    in_edge.get_channels_on_target().get_elements();
					for (auto const out_edge_descriptor :
					     m_topology.get_reference().out_edges(partitioned_vertex_descriptor)) {
						auto const target_vertex_descriptor =
						    m_topology.get_reference().target(out_edge_descriptor);
						if (auto const population_post =
						        dynamic_cast<grenade::common::Population const*>(
						            &m_topology.get_reference().get(
						                m_topology.get_reference().target(out_edge_descriptor)));
						    population_post) {
							auto const neuron_post =
							    dynamic_cast<abstract::LocallyPlacedNeuron const*>(
							        &population_post->get_cell());
							if (!neuron_post) {
								continue;
							}
							auto const& out_edge =
							    m_topology.get_reference().get(out_edge_descriptor);
							auto const population_post_shape_elements =
							    population_post->get_shape().get_elements();

							std::set<size_t> neuron_post_neuron_dimensions;
							std::set<size_t> neuron_post_compartment_dimensions;
							std::set<size_t> neuron_post_receptor_dimensions;
							auto const neuron_post_input_port =
							    population_post->get_input_ports().at(out_edge.port_on_target);
							auto const& neuron_post_channels =
							    neuron_post_input_port.get_channels();
							auto const neuron_post_channels_dimension_units =
							    neuron_post_channels.get_dimension_units();
							for (size_t i = 0; i < neuron_post_channels.dimensionality(); ++i) {
								if (dynamic_cast<
								        grenade::common::CellOnPopulationDimensionUnit const*>(
								        &(*neuron_post_channels_dimension_units.at(i)))) {
									neuron_post_neuron_dimensions.insert(i);
								} else if (dynamic_cast<
								               grenade::common::
								                   CompartmentOnNeuronDimensionUnit const*>(
								               &(*neuron_post_channels_dimension_units.at(i)))) {
									neuron_post_compartment_dimensions.insert(i);
								} else if (dynamic_cast<
								               grenade::common::
								                   ReceptorOnCompartmentDimensionUnit const*>(
								               &(*neuron_post_channels_dimension_units.at(i)))) {
									neuron_post_receptor_dimensions.insert(i);
								}
							}
							assert(neuron_post_compartment_dimensions.size() == 1);
							size_t const neuron_post_compartment_dimension =
							    *neuron_post_compartment_dimensions.begin();
							assert(neuron_post_receptor_dimensions.size() == 1);
							size_t const neuron_post_receptor_dimension =
							    *neuron_post_receptor_dimensions.begin();
							std::set<size_t> synapse_output_dimensions;
							for (size_t i = 0; i < projection->get_connector()
							                           .get_output_sequence()
							                           ->dimensionality();
							     ++i) {
								synapse_output_dimensions.insert(
								    i + projection->get_connector()
								            .get_input_sequence()
								            ->dimensionality());
							}
							auto synapse_input_channels = projection->get_synapse()
							                                  .get_input_ports()
							                                  .projection.at(in_edge.port_on_target)
							                                  .get_channels()
							                                  .get_elements();
							auto synapse_output_channels =
							    projection->get_synapse()
							        .get_output_ports()
							        .projection.at(out_edge.port_on_source)
							        .get_channels()
							        .get_elements();
							if (synapse_input_channels.empty()) {
								synapse_input_channels.push_back(grenade::common::MultiIndex());
							}
							if (synapse_output_channels.empty()) {
								synapse_output_channels.push_back(grenade::common::MultiIndex());
							}
							std::vector<grenade::common::MultiIndex> elements;
							std::vector<grenade::common::MultiIndex> synapse_indices;
							auto const synapse_sequence_elements = synapse_sequence->get_elements();
							for (size_t i = 0;
							     auto const& synapse_sequence_element : synapse_sequence_elements) {
								for (auto const& synapse_input_channel : synapse_input_channels) {
									for (auto const& synapse_output_channel :
									     synapse_output_channels) {
										grenade::common::MultiIndex element;
										for (auto const& synapse_input_dimension :
										     synapse_input_dimensions) {
											element.value.push_back(
											    synapse_sequence_element.value.at(
											        synapse_input_dimension));
										}
										element.value.insert(
										    element.value.end(),
										    synapse_input_channel.value.begin(),
										    synapse_input_channel.value.end());
										for (auto const& synapse_output_dimension :
										     synapse_output_dimensions) {
											element.value.push_back(
											    synapse_sequence_element.value.at(
											        synapse_output_dimension));
										}
										element.value.insert(
										    element.value.end(),
										    synapse_output_channel.value.begin(),
										    synapse_output_channel.value.end());
										elements.push_back(element);
										synapse_indices.push_back(grenade::common::MultiIndex({i}));
									}
								}
								i++;
							}
							grenade::common::MultiIndexSequence::DimensionUnits dimension_units;
							auto const input_dimension_units =
							    in_edge.get_channels_on_target().get_dimension_units();
							dimension_units.insert(
							    dimension_units.end(), input_dimension_units.begin(),
							    input_dimension_units.end());
							auto const output_dimension_units =
							    out_edge.get_channels_on_source().get_dimension_units();
							dimension_units.insert(
							    dimension_units.end(), output_dimension_units.begin(),
							    output_dimension_units.end());
							grenade::common::ListMultiIndexSequence projection_sequence(
							    std::move(elements), std::move(dimension_units));
							grenade::common::ListMultiIndexSequence
							    projection_synapse_index_sequence(std::move(synapse_indices));

							if (projection_sequence.size() == 0) {
								continue;
							}
							auto const local_projection_sequence =
							    projection_sequence.subset_restriction(
							        *in_edge.get_channels_on_target().cartesian_product(
							            out_edge.get_channels_on_source()));

							auto const local_projection_synapse_index_sequence =
							    projection_synapse_index_sequence
							        .related_sequence_subset_restriction(
							            projection_sequence,
							            *in_edge.get_channels_on_target().cartesian_product(
							                out_edge.get_channels_on_source()))
							        ->get_elements();

							std::set<size_t> projection_input_dimensions;
							for (size_t i = 0;
							     i < in_edge.get_channels_on_target().dimensionality(); ++i) {
								projection_input_dimensions.insert(i);
							}

							auto const local_input_synapse_sequence_elements =
							    local_projection_sequence->projection(projection_input_dimensions)
							        ->get_elements();

							std::set<size_t> projection_output_dimensions;
							for (size_t i = 0;
							     i < out_edge.get_channels_on_source().dimensionality(); ++i) {
								projection_output_dimensions.insert(
								    i + in_edge.get_channels_on_target().dimensionality());
							}
							auto const local_output_synapse_sequence_elements =
							    local_projection_sequence->projection(projection_output_dimensions)
							        ->get_elements();

							auto const local_out_edge_channels_on_target_elements =
							    out_edge.get_channels_on_target().get_elements();
							auto const local_out_edge_channels_on_source_elements =
							    out_edge.get_channels_on_source().get_elements();
							for (size_t i = 0; i < local_input_synapse_sequence_elements.size();
							     ++i) {
								size_t const local_neuron_pre_index = std::distance(
								    local_in_edge_channels_on_target_elements.begin(),
								    std::find(
								        local_in_edge_channels_on_target_elements.begin(),
								        local_in_edge_channels_on_target_elements.end(),
								        local_input_synapse_sequence_elements.at(i)));
								auto const& local_neuron_pre =
								    local_in_edge_channels_on_source_elements.at(
								        local_neuron_pre_index);
								grenade::common::MultiIndex local_neuron_pre_in_shape;
								for (auto const& dim : neuron_pre_neuron_dimensions) {
									local_neuron_pre_in_shape.value.push_back(
									    local_neuron_pre.value.at(dim));
								}
								size_t const global_neuron_pre_index = std::distance(
								    population_pre_shape_elements.begin(),
								    std::find(
								        population_pre_shape_elements.begin(),
								        population_pre_shape_elements.end(),
								        local_neuron_pre_in_shape));
								size_t const local_neuron_post_index = std::distance(
								    local_out_edge_channels_on_source_elements.begin(),
								    std::find(
								        local_out_edge_channels_on_source_elements.begin(),
								        local_out_edge_channels_on_source_elements.end(),
								        local_output_synapse_sequence_elements.at(i)));
								auto const& local_neuron_post =
								    local_out_edge_channels_on_target_elements.at(
								        local_neuron_post_index);
								grenade::common::MultiIndex local_neuron_post_in_shape;
								for (auto const& dim : neuron_post_neuron_dimensions) {
									local_neuron_post_in_shape.value.push_back(
									    local_neuron_post.value.at(dim));
								}
								size_t const global_neuron_post_index = std::distance(
								    population_post_shape_elements.begin(),
								    std::find(
								        population_post_shape_elements.begin(),
								        population_post_shape_elements.end(),
								        local_neuron_post_in_shape));
								auto const& spike_master_pre =
								    neuron_pre->compartments
								        .at(grenade::common::CompartmentOnNeuron(
								            local_neuron_pre.value.at(
								                neuron_pre_compartment_dimension)))
								        .spike_master;
								assert(spike_master_pre);
								auto const neuron_post_compartment =
								    grenade::common::CompartmentOnNeuron(local_neuron_post.value.at(
								        neuron_post_compartment_dimension));
								auto const neuron_post_receptor =
								    grenade::common::ReceptorOnCompartment(
								        local_neuron_post.value.at(neuron_post_receptor_dimension));
								for (auto const& an_target :
								     m_connection_routing_result.at(partitioned_vertex_descriptor)
								         .at(i)
								         .atomic_neurons_on_target_compartment.at(
								             neuron_post_receptor)) {
									ret.push_back(
									    {halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
									         neuron_pre->shape, anchors.at(source_vertex_descriptor)
									                                .at(global_neuron_pre_index))
									         .get_placed_compartments()
									         .at(halco::hicann_dls::vx::v3::
									                 CompartmentOnLogicalNeuron(
									                     local_neuron_pre.value.at(
									                         neuron_pre_compartment_dimension)))
									         .at(spike_master_pre->atomic_neuron_on_compartment),
									     {m_topology.get_reference().source(in_edge_descriptor),
									      global_neuron_pre_index,
									      halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron(
									          local_neuron_pre.value.at(
									              neuron_pre_compartment_dimension))},
									     halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
									         neuron_post->shape,
									         anchors.at(target_vertex_descriptor)
									             .at(global_neuron_post_index))
									         .get_placed_compartments()
									         .at(halco::hicann_dls::vx::v3::
									                 CompartmentOnLogicalNeuron(
									                     neuron_post_compartment))
									         .at(an_target),
									     {partitioned_vertex_descriptor,
									      local_projection_synapse_index_sequence.at(i).value.at(
									          0)},
									     neuron_post->compartments.at(neuron_post_compartment)
									         .receptors.at(an_target)
									         .at(neuron_post_receptor)});
								}
							}
						}
					}
				}
			} else {
				throw std::runtime_error("Connector type not supported.");
			}
		}
	}
	m_internal_connections = ret;
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
	if (!m_background_connections.empty()) {
		return m_background_connections;
	}

	auto const anchors = get_anchors();

	std::vector<BackgroundConnection> ret;
	for (auto const& partitioned_vertex_descriptor : m_partitioned_vertex_descriptors) {
		if (auto const projection = dynamic_cast<grenade::common::Projection const*>(
		        &m_topology.get_reference().get(partitioned_vertex_descriptor))) {
			if (auto const static_connector = dynamic_cast<grenade::common::StaticConnector const*>(
			        &projection->get_connector());
			    static_connector) {
				auto const section =
				    projection->get_connector().get_input_sequence()->cartesian_product(
				        *projection->get_connector().get_output_sequence());
				auto const synapse_sequence = static_connector->get_synapse_connections(*section);

				for (auto const in_edge_descriptor :
				     m_topology.get_reference().in_edges(partitioned_vertex_descriptor)) {
					auto const source_vertex_descriptor =
					    m_topology.get_reference().source(in_edge_descriptor);
					if (std::find(
					        m_partitioned_vertex_descriptors.begin(),
					        m_partitioned_vertex_descriptors.end(),
					        source_vertex_descriptor) == m_partitioned_vertex_descriptors.end()) {
						// source not on same execution instance
						continue;
					}
					auto const& population_pre = dynamic_cast<grenade::common::Population const&>(
					    m_topology.get_reference().get(source_vertex_descriptor));
					auto const neuron_pre = dynamic_cast<abstract::PoissonSourceNeuron const*>(
					    &population_pre.get_cell());
					if (!neuron_pre) {
						continue;
					}
					std::map<
					    halco::hicann_dls::vx::v3::HemisphereOnDLS,
					    halco::hicann_dls::vx::v3::BackgroundSpikeSourceOnDLS>
					    population_pre_locations;
					for (auto const& partitioned_vertex_descriptor :
					     m_partitioned_vertex_descriptors) {
						for (auto const& inter_graph_hyper_edge_descriptor :

						     m_topology.inter_graph_hyper_edges_by_reference(
						         partitioned_vertex_descriptor)) {
							auto const& inter_graph_hyper_edge =
							    m_topology.get(inter_graph_hyper_edge_descriptor);
							if (auto const poisson_source_mapping =
							        dynamic_cast<abstract::PoissonSourceNeuronMapping const*>(
							            &inter_graph_hyper_edge);
							    poisson_source_mapping) {
								for (auto const mapped_vertex_descriptor :
								     m_topology.links(inter_graph_hyper_edge_descriptor)) {
									auto const& background_source_vertex = dynamic_cast<
									    signal_flow::vertex::BackgroundSpikeSource const&>(
									    m_topology.get(mapped_vertex_descriptor));
									population_pre_locations.emplace(
									    background_source_vertex.coordinate.toPADIBusOnDLS()
									        .toPADIBusBlockOnDLS()
									        .toHemisphereOnDLS(),
									    background_source_vertex.coordinate);
								}
							}
						}
					}

					auto const& in_edge = m_topology.get_reference().get(in_edge_descriptor);
					auto const population_pre_shape_elements =
					    population_pre.get_shape().get_elements();

					std::set<size_t> synapse_input_dimensions;
					for (size_t i = 0;
					     i < projection->get_connector().get_input_sequence()->dimensionality();
					     ++i) {
						synapse_input_dimensions.insert(i);
					}

					std::set<size_t> neuron_pre_neuron_dimensions;
					std::set<size_t> neuron_pre_compartment_dimensions;
					auto const& neuron_pre_ports = population_pre.get_output_ports();
					auto const& neuron_pre_channels =
					    neuron_pre_ports.at(in_edge.port_on_source).get_channels();
					for (size_t i = 0; i < neuron_pre_channels.dimensionality(); ++i) {
						if (dynamic_cast<grenade::common::CellOnPopulationDimensionUnit const*>(
						        &(*neuron_pre_channels.get_dimension_units().at(i)))) {
							neuron_pre_neuron_dimensions.insert(i);
						} else if (dynamic_cast<
						               grenade::common::CompartmentOnNeuronDimensionUnit const*>(
						               &(*neuron_pre_channels.get_dimension_units().at(i)))) {
							neuron_pre_compartment_dimensions.insert(i);
						}
					}
					assert(neuron_pre_compartment_dimensions.size() == 1);
					auto const local_in_edge_channels_on_source_elements =
					    in_edge.get_channels_on_source().get_elements();
					auto const local_in_edge_channels_on_target_elements =
					    in_edge.get_channels_on_target().get_elements();
					for (auto const out_edge_descriptor :
					     m_topology.get_reference().out_edges(partitioned_vertex_descriptor)) {
						auto const target_vertex_descriptor =
						    m_topology.get_reference().target(out_edge_descriptor);
						if (auto const population_post =
						        dynamic_cast<grenade::common::Population const*>(
						            &m_topology.get_reference().get(
						                m_topology.get_reference().target(out_edge_descriptor)));
						    population_post) {
							auto const neuron_post =
							    dynamic_cast<abstract::LocallyPlacedNeuron const*>(
							        &population_post->get_cell());
							if (!neuron_post) {
								continue;
							}
							auto const& out_edge =
							    m_topology.get_reference().get(out_edge_descriptor);
							auto const population_post_shape_elements =
							    population_post->get_shape().get_elements();
							std::set<size_t> neuron_post_neuron_dimensions;
							std::set<size_t> neuron_post_compartment_dimensions;
							std::set<size_t> neuron_post_receptor_dimensions;
							auto const neuron_post_ports = population_post->get_input_ports();
							auto const& neuron_post_channels =
							    neuron_post_ports.at(out_edge.port_on_target).get_channels();
							auto const neuron_post_channels_dimension_units =
							    neuron_post_channels.get_dimension_units();
							for (size_t i = 0; i < neuron_post_channels.dimensionality(); ++i) {
								if (dynamic_cast<
								        grenade::common::CellOnPopulationDimensionUnit const*>(
								        &(*neuron_post_channels_dimension_units.at(i)))) {
									neuron_post_neuron_dimensions.insert(i);
								} else if (dynamic_cast<
								               grenade::common::
								                   CompartmentOnNeuronDimensionUnit const*>(
								               &(*neuron_post_channels_dimension_units.at(i)))) {
									neuron_post_compartment_dimensions.insert(i);
								} else if (dynamic_cast<
								               grenade::common::
								                   ReceptorOnCompartmentDimensionUnit const*>(
								               &(*neuron_post_channels_dimension_units.at(i)))) {
									neuron_post_receptor_dimensions.insert(i);
								}
							}
							assert(neuron_post_compartment_dimensions.size() == 1);
							size_t const neuron_post_compartment_dimension =
							    *neuron_post_compartment_dimensions.begin();
							assert(neuron_post_receptor_dimensions.size() == 1);
							size_t const neuron_post_receptor_dimension =
							    *neuron_post_receptor_dimensions.begin();
							std::set<size_t> synapse_output_dimensions;
							for (size_t i = 0; i < projection->get_connector()
							                           .get_output_sequence()
							                           ->dimensionality();
							     ++i) {
								synapse_output_dimensions.insert(
								    i + projection->get_connector()
								            .get_input_sequence()
								            ->dimensionality());
							}
							auto synapse_input_channels = projection->get_synapse()
							                                  .get_input_ports()
							                                  .projection.at(in_edge.port_on_target)
							                                  .get_channels()
							                                  .get_elements();
							auto synapse_output_channels =
							    projection->get_synapse()
							        .get_output_ports()
							        .projection.at(out_edge.port_on_source)
							        .get_channels()
							        .get_elements();
							if (synapse_input_channels.empty()) {
								synapse_input_channels.push_back(grenade::common::MultiIndex());
							}
							if (synapse_output_channels.empty()) {
								synapse_output_channels.push_back(grenade::common::MultiIndex());
							}
							std::vector<grenade::common::MultiIndex> elements;
							std::vector<grenade::common::MultiIndex> synapse_indices;
							auto const synapse_sequence_elements = synapse_sequence->get_elements();
							for (size_t i = 0;
							     auto const& synapse_sequence_element : synapse_sequence_elements) {
								for (auto const& synapse_input_channel : synapse_input_channels) {
									for (auto const& synapse_output_channel :
									     synapse_output_channels) {
										grenade::common::MultiIndex element;
										for (auto const& synapse_input_dimension :
										     synapse_input_dimensions) {
											element.value.push_back(
											    synapse_sequence_element.value.at(
											        synapse_input_dimension));
										}
										element.value.insert(
										    element.value.end(),
										    synapse_input_channel.value.begin(),
										    synapse_input_channel.value.end());
										for (auto const& synapse_output_dimension :
										     synapse_output_dimensions) {
											element.value.push_back(
											    synapse_sequence_element.value.at(
											        synapse_output_dimension));
										}
										element.value.insert(
										    element.value.end(),
										    synapse_output_channel.value.begin(),
										    synapse_output_channel.value.end());
										elements.push_back(element);
										synapse_indices.push_back(grenade::common::MultiIndex({i}));
									}
								}
								i++;
							}
							grenade::common::MultiIndexSequence::DimensionUnits dimension_units;
							auto const input_dimension_units =
							    in_edge.get_channels_on_target().get_dimension_units();
							dimension_units.insert(
							    dimension_units.end(), input_dimension_units.begin(),
							    input_dimension_units.end());
							auto const output_dimension_units =
							    out_edge.get_channels_on_source().get_dimension_units();
							dimension_units.insert(
							    dimension_units.end(), output_dimension_units.begin(),
							    output_dimension_units.end());
							grenade::common::ListMultiIndexSequence projection_sequence(
							    std::move(elements), std::move(dimension_units));
							grenade::common::ListMultiIndexSequence
							    projection_synapse_index_sequence(std::move(synapse_indices));

							if (projection_sequence.size() == 0) {
								continue;
							}
							auto const local_projection_sequence =
							    projection_sequence.subset_restriction(
							        *in_edge.get_channels_on_target().cartesian_product(
							            out_edge.get_channels_on_source()));

							auto const local_projection_synapse_index_sequence =
							    projection_synapse_index_sequence
							        .related_sequence_subset_restriction(
							            projection_sequence,
							            *in_edge.get_channels_on_target().cartesian_product(
							                out_edge.get_channels_on_source()))
							        ->get_elements();

							std::set<size_t> projection_input_dimensions;
							for (size_t i = 0;
							     i < in_edge.get_channels_on_target().dimensionality(); ++i) {
								projection_input_dimensions.insert(i);
							}

							auto const local_input_synapse_sequence_elements =
							    local_projection_sequence->projection(projection_input_dimensions)
							        ->get_elements();

							std::set<size_t> projection_output_dimensions;
							for (size_t i = 0;
							     i < out_edge.get_channels_on_source().dimensionality(); ++i) {
								projection_output_dimensions.insert(
								    i + in_edge.get_channels_on_target().dimensionality());
							}
							auto const local_output_synapse_sequence_elements =
							    local_projection_sequence->projection(projection_output_dimensions)
							        ->get_elements();

							auto const local_out_edge_channels_on_target_elements =
							    out_edge.get_channels_on_target().get_elements();
							auto const local_out_edge_channels_on_source_elements =
							    out_edge.get_channels_on_source().get_elements();
							for (size_t i = 0; i < local_input_synapse_sequence_elements.size();
							     ++i) {
								size_t const local_neuron_pre_index = std::distance(
								    local_in_edge_channels_on_target_elements.begin(),
								    std::find(
								        local_in_edge_channels_on_target_elements.begin(),
								        local_in_edge_channels_on_target_elements.end(),
								        local_input_synapse_sequence_elements.at(i)));
								auto const& local_neuron_pre =
								    local_in_edge_channels_on_source_elements.at(
								        local_neuron_pre_index);
								grenade::common::MultiIndex local_neuron_pre_in_shape;
								for (auto const& dim : neuron_pre_neuron_dimensions) {
									local_neuron_pre_in_shape.value.push_back(
									    local_neuron_pre.value.at(dim));
								}
								size_t const global_neuron_pre_index = std::distance(
								    population_pre_shape_elements.begin(),
								    std::find(
								        population_pre_shape_elements.begin(),
								        population_pre_shape_elements.end(),
								        local_neuron_pre_in_shape));
								size_t const local_neuron_post_index = std::distance(
								    local_out_edge_channels_on_source_elements.begin(),
								    std::find(
								        local_out_edge_channels_on_source_elements.begin(),
								        local_out_edge_channels_on_source_elements.end(),
								        local_output_synapse_sequence_elements.at(i)));
								auto const& local_neuron_post =
								    local_out_edge_channels_on_target_elements.at(
								        local_neuron_post_index);
								grenade::common::MultiIndex local_neuron_post_in_shape;
								for (auto const& dim : neuron_post_neuron_dimensions) {
									local_neuron_post_in_shape.value.push_back(
									    local_neuron_post.value.at(dim));
								}
								size_t const global_neuron_post_index = std::distance(
								    population_post_shape_elements.begin(),
								    std::find(
								        population_post_shape_elements.begin(),
								        population_post_shape_elements.end(),
								        local_neuron_post_in_shape));
								auto const neuron_post_compartment =
								    grenade::common::CompartmentOnNeuron(local_neuron_post.value.at(
								        neuron_post_compartment_dimension));
								auto const neuron_post_receptor =
								    grenade::common::ReceptorOnCompartment(
								        local_neuron_post.value.at(neuron_post_receptor_dimension));
								for (auto const& an_target :
								     m_connection_routing_result.at(partitioned_vertex_descriptor)
								         .at(i)
								         .atomic_neurons_on_target_compartment.at(
								             neuron_post_receptor)) {
									ret.push_back(
									    {population_pre_locations.at(
									         halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
									             neuron_post->shape,
									             anchors.at(target_vertex_descriptor)
									                 .at(global_neuron_post_index))
									             .get_placed_compartments()
									             .at(halco::hicann_dls::vx::v3::
									                     CompartmentOnLogicalNeuron(
									                         neuron_post_compartment))
									             .at(an_target)
									             .toNeuronRowOnDLS()
									             .toHemisphereOnDLS()),
									     {m_topology.get_reference().source(in_edge_descriptor),
									      global_neuron_pre_index,
									      halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron()},
									     halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
									         neuron_post->shape,
									         anchors.at(target_vertex_descriptor)
									             .at(global_neuron_post_index))
									         .get_placed_compartments()
									         .at(halco::hicann_dls::vx::v3::
									                 CompartmentOnLogicalNeuron(
									                     neuron_post_compartment))
									         .at(an_target),
									     {partitioned_vertex_descriptor,
									      local_projection_synapse_index_sequence.at(i).value.at(
									          0)},
									     neuron_post->compartments.at(neuron_post_compartment)
									         .receptors.at(an_target)
									         .at(neuron_post_receptor)});
								}
							}
						}
					}
				}
			} else {
				throw std::runtime_error("Connector type not supported.");
			}
		}
	}
	m_background_connections = ret;
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
	if (!m_external_connections.empty()) {
		return m_external_connections;
	}

	auto const anchors = get_anchors();

	std::vector<ExternalConnection> ret;
	for (auto const& partitioned_vertex_descriptor : m_partitioned_vertex_descriptors) {
		if (auto const projection = dynamic_cast<grenade::common::Projection const*>(
		        &m_topology.get_reference().get(partitioned_vertex_descriptor));
		    projection) {
			if (auto const static_connector = dynamic_cast<grenade::common::StaticConnector const*>(
			        &projection->get_connector());
			    static_connector) {
				auto const section =
				    projection->get_connector().get_input_sequence()->cartesian_product(
				        *projection->get_connector().get_output_sequence());
				auto const synapse_sequence = static_connector->get_synapse_connections(*section);

				for (auto const in_edge_descriptor :
				     m_topology.get_reference().in_edges(partitioned_vertex_descriptor)) {
					auto const& population_pre = dynamic_cast<grenade::common::Population const&>(
					    m_topology.get_reference().get(
					        m_topology.get_reference().source(in_edge_descriptor)));
					// TODO(SpikeIO): add SpikeIOInputNeuron case
					auto const external_neuron_pre =
					    dynamic_cast<abstract::ExternalSourceNeuron const*>(
					        &population_pre.get_cell());
					if (!external_neuron_pre) {
						// source not external source
						continue;
					}
					auto const& in_edge = m_topology.get_reference().get(in_edge_descriptor);
					auto const population_pre_shape_elements =
					    population_pre.get_shape().get_elements();

					std::set<size_t> synapse_input_dimensions;
					for (size_t i = 0;
					     i < projection->get_connector().get_input_sequence()->dimensionality();
					     ++i) {
						synapse_input_dimensions.insert(i);
					}

					std::set<size_t> neuron_pre_neuron_dimensions;
					std::set<size_t> neuron_pre_compartment_dimensions;
					auto const neuron_pre_output_ports = population_pre.get_output_ports();
					auto const& neuron_pre_channels =
					    neuron_pre_output_ports.at(in_edge.port_on_source).get_channels();
					for (size_t i = 0; i < neuron_pre_channels.dimensionality(); ++i) {
						if (dynamic_cast<grenade::common::CellOnPopulationDimensionUnit const*>(
						        &(*neuron_pre_channels.get_dimension_units().at(i)))) {
							neuron_pre_neuron_dimensions.insert(i);
						} else if (dynamic_cast<
						               grenade::common::CompartmentOnNeuronDimensionUnit const*>(
						               &(*neuron_pre_channels.get_dimension_units().at(i)))) {
							neuron_pre_compartment_dimensions.insert(i);
						}
					}
					assert(neuron_pre_compartment_dimensions.size() == 1);
					auto const local_in_edge_channels_on_source_elements =
					    in_edge.get_channels_on_source().get_elements();
					auto const local_in_edge_channels_on_target_elements =
					    in_edge.get_channels_on_target().get_elements();
					for (auto const out_edge_descriptor :
					     m_topology.get_reference().out_edges(partitioned_vertex_descriptor)) {
						auto const target_vertex_descriptor =
						    m_topology.get_reference().target(out_edge_descriptor);
						if (auto const population_post =
						        dynamic_cast<grenade::common::Population const*>(
						            &m_topology.get_reference().get(
						                m_topology.get_reference().target(out_edge_descriptor)));
						    population_post) {
							auto const neuron_post =
							    dynamic_cast<abstract::LocallyPlacedNeuron const*>(
							        &population_post->get_cell());
							if (!neuron_post) {
								continue;
							}
							auto const& out_edge =
							    m_topology.get_reference().get(out_edge_descriptor);
							auto const population_post_shape_elements =
							    population_post->get_shape().get_elements();
							std::set<size_t> neuron_post_neuron_dimensions;
							std::set<size_t> neuron_post_compartment_dimensions;
							std::set<size_t> neuron_post_receptor_dimensions;
							auto const neuron_post_input_ports = population_post->get_input_ports();
							auto const& neuron_post_channels =
							    neuron_post_input_ports.at(out_edge.port_on_target).get_channels();
							auto const neuron_post_channels_dimension_units =
							    neuron_post_channels.get_dimension_units();
							for (size_t i = 0; i < neuron_post_channels.dimensionality(); ++i) {
								if (dynamic_cast<
								        grenade::common::CellOnPopulationDimensionUnit const*>(
								        &(*neuron_post_channels_dimension_units.at(i)))) {
									neuron_post_neuron_dimensions.insert(i);
								} else if (dynamic_cast<
								               grenade::common::
								                   CompartmentOnNeuronDimensionUnit const*>(
								               &(*neuron_post_channels_dimension_units.at(i)))) {
									neuron_post_compartment_dimensions.insert(i);
								} else if (dynamic_cast<
								               grenade::common::
								                   ReceptorOnCompartmentDimensionUnit const*>(
								               &(*neuron_post_channels_dimension_units.at(i)))) {
									neuron_post_receptor_dimensions.insert(i);
								}
							}
							assert(neuron_post_compartment_dimensions.size() == 1);
							size_t const neuron_post_compartment_dimension =
							    *neuron_post_compartment_dimensions.begin();
							assert(neuron_post_receptor_dimensions.size() == 1);
							size_t const neuron_post_receptor_dimension =
							    *neuron_post_receptor_dimensions.begin();
							std::set<size_t> synapse_output_dimensions;
							for (size_t i = 0; i < projection->get_connector()
							                           .get_output_sequence()
							                           ->dimensionality();
							     ++i) {
								synapse_output_dimensions.insert(
								    i + projection->get_connector()
								            .get_input_sequence()
								            ->dimensionality());
							}
							auto synapse_input_channels = projection->get_synapse()
							                                  .get_input_ports()
							                                  .projection.at(in_edge.port_on_target)
							                                  .get_channels()
							                                  .get_elements();
							auto synapse_output_channels =
							    projection->get_synapse()
							        .get_output_ports()
							        .projection.at(out_edge.port_on_source)
							        .get_channels()
							        .get_elements();
							if (synapse_input_channels.empty()) {
								synapse_input_channels.push_back(grenade::common::MultiIndex());
							}
							if (synapse_output_channels.empty()) {
								synapse_output_channels.push_back(grenade::common::MultiIndex());
							}
							std::vector<grenade::common::MultiIndex> elements;
							std::vector<grenade::common::MultiIndex> synapse_indices;
							auto const synapse_sequence_elements = synapse_sequence->get_elements();
							for (size_t i = 0;
							     auto const& synapse_sequence_element : synapse_sequence_elements) {
								for (auto const& synapse_input_channel : synapse_input_channels) {
									for (auto const& synapse_output_channel :
									     synapse_output_channels) {
										grenade::common::MultiIndex element;
										for (auto const& synapse_input_dimension :
										     synapse_input_dimensions) {
											element.value.push_back(
											    synapse_sequence_element.value.at(
											        synapse_input_dimension));
										}
										element.value.insert(
										    element.value.end(),
										    synapse_input_channel.value.begin(),
										    synapse_input_channel.value.end());
										for (auto const& synapse_output_dimension :
										     synapse_output_dimensions) {
											element.value.push_back(
											    synapse_sequence_element.value.at(
											        synapse_output_dimension));
										}
										element.value.insert(
										    element.value.end(),
										    synapse_output_channel.value.begin(),
										    synapse_output_channel.value.end());
										elements.push_back(element);
										synapse_indices.push_back(grenade::common::MultiIndex({i}));
									}
								}
								i++;
							}
							grenade::common::MultiIndexSequence::DimensionUnits dimension_units;
							auto const input_dimension_units =
							    in_edge.get_channels_on_target().get_dimension_units();
							dimension_units.insert(
							    dimension_units.end(), input_dimension_units.begin(),
							    input_dimension_units.end());
							auto const output_dimension_units =
							    out_edge.get_channels_on_source().get_dimension_units();
							dimension_units.insert(
							    dimension_units.end(), output_dimension_units.begin(),
							    output_dimension_units.end());
							grenade::common::ListMultiIndexSequence projection_sequence(
							    std::move(elements), std::move(dimension_units));
							grenade::common::ListMultiIndexSequence
							    projection_synapse_index_sequence(std::move(synapse_indices));

							if (projection_sequence.size() == 0) {
								continue;
							}
							auto const local_projection_sequence =
							    projection_sequence.subset_restriction(
							        *in_edge.get_channels_on_target().cartesian_product(
							            out_edge.get_channels_on_source()));

							auto const local_projection_synapse_index_sequence =
							    projection_synapse_index_sequence
							        .related_sequence_subset_restriction(
							            projection_sequence,
							            *in_edge.get_channels_on_target().cartesian_product(
							                out_edge.get_channels_on_source()))
							        ->get_elements();

							std::set<size_t> projection_input_dimensions;
							for (size_t i = 0;
							     i < in_edge.get_channels_on_target().dimensionality(); ++i) {
								projection_input_dimensions.insert(i);
							}

							auto const local_input_synapse_sequence_elements =
							    local_projection_sequence->projection(projection_input_dimensions)
							        ->get_elements();

							std::set<size_t> projection_output_dimensions;
							for (size_t i = 0;
							     i < out_edge.get_channels_on_source().dimensionality(); ++i) {
								projection_output_dimensions.insert(
								    i + in_edge.get_channels_on_target().dimensionality());
							}
							auto const local_output_synapse_sequence_elements =
							    local_projection_sequence->projection(projection_output_dimensions)
							        ->get_elements();

							auto const local_out_edge_channels_on_target_elements =
							    out_edge.get_channels_on_target().get_elements();
							auto const local_out_edge_channels_on_source_elements =
							    out_edge.get_channels_on_source().get_elements();
							for (size_t i = 0; i < local_input_synapse_sequence_elements.size();
							     ++i) {
								size_t const local_neuron_pre_index = std::distance(
								    local_in_edge_channels_on_target_elements.begin(),
								    std::find(
								        local_in_edge_channels_on_target_elements.begin(),
								        local_in_edge_channels_on_target_elements.end(),
								        local_input_synapse_sequence_elements.at(i)));
								auto const& local_neuron_pre =
								    local_in_edge_channels_on_source_elements.at(
								        local_neuron_pre_index);
								grenade::common::MultiIndex local_neuron_pre_in_shape;
								for (auto const& dim : neuron_pre_neuron_dimensions) {
									local_neuron_pre_in_shape.value.push_back(
									    local_neuron_pre.value.at(dim));
								}
								size_t const global_neuron_pre_index = std::distance(
								    population_pre_shape_elements.begin(),
								    std::find(
								        population_pre_shape_elements.begin(),
								        population_pre_shape_elements.end(),
								        local_neuron_pre_in_shape));
								size_t const local_neuron_post_index = std::distance(
								    local_out_edge_channels_on_source_elements.begin(),
								    std::find(
								        local_out_edge_channels_on_source_elements.begin(),
								        local_out_edge_channels_on_source_elements.end(),
								        local_output_synapse_sequence_elements.at(i)));
								auto const& local_neuron_post =
								    local_out_edge_channels_on_target_elements.at(
								        local_neuron_post_index);
								grenade::common::MultiIndex local_neuron_post_in_shape;
								for (auto const& dim : neuron_post_neuron_dimensions) {
									local_neuron_post_in_shape.value.push_back(
									    local_neuron_post.value.at(dim));
								}
								size_t const global_neuron_post_index = std::distance(
								    population_post_shape_elements.begin(),
								    std::find(
								        population_post_shape_elements.begin(),
								        population_post_shape_elements.end(),
								        local_neuron_post_in_shape));
								auto const neuron_post_compartment =
								    grenade::common::CompartmentOnNeuron(local_neuron_post.value.at(
								        neuron_post_compartment_dimension));
								auto const neuron_post_receptor =
								    grenade::common::ReceptorOnCompartment(
								        local_neuron_post.value.at(neuron_post_receptor_dimension));
								for (auto const& an_target :
								     m_connection_routing_result.at(partitioned_vertex_descriptor)
								         .at(i)
								         .atomic_neurons_on_target_compartment.at(
								             neuron_post_receptor)) {
									ret.push_back(
									    {{m_topology.get_reference().source(in_edge_descriptor),
									      global_neuron_pre_index,
									      halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron()},
									     halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
									         neuron_post->shape,
									         anchors.at(target_vertex_descriptor)
									             .at(global_neuron_post_index))
									         .get_placed_compartments()
									         .at(halco::hicann_dls::vx::v3::
									                 CompartmentOnLogicalNeuron(
									                     neuron_post_compartment))
									         .at(an_target),
									     {partitioned_vertex_descriptor,
									      local_projection_synapse_index_sequence.at(i).value.at(
									          0)},
									     neuron_post->compartments.at(neuron_post_compartment)
									         .receptors.at(an_target)
									         .at(neuron_post_receptor)});
								}
							}
						}
					}
				}
			} else {
				throw std::runtime_error("Connector type not supported.");
			}
		}
	}
	m_external_connections = ret;
	return ret;
}

halco::common::typed_array<
    std::map<Receptor::Type, std::vector<std::pair<grenade::common::VertexOnTopology, size_t>>>,
    halco::hicann_dls::vx::v3::HemisphereOnDLS>
RoutingConstraints::get_external_connections_per_hemisphere() const
{
	halco::common::typed_array<
	    std::map<Receptor::Type, std::vector<std::pair<grenade::common::VertexOnTopology, size_t>>>,
	    halco::hicann_dls::vx::v3::HemisphereOnDLS>
	    ret;

	for (auto const& connection : get_external_connections()) {
		ret[connection.target.toNeuronRowOnDLS().toHemisphereOnDLS()][connection.receptor_type]
		    .push_back(connection.descriptor);
	}
	return ret;
}

halco::common::typed_array<
    std::map<Receptor::Type, std::set<std::pair<grenade::common::VertexOnTopology, size_t>>>,
    halco::hicann_dls::vx::v3::HemisphereOnDLS>
RoutingConstraints::get_external_sources_to_hemisphere() const
{
	auto const external_connections = get_external_connections();

	halco::common::typed_array<
	    std::map<Receptor::Type, std::set<std::pair<grenade::common::VertexOnTopology, size_t>>>,
	    halco::hicann_dls::vx::v3::HemisphereOnDLS>
	    ret;

	for (auto const& connection : external_connections) {
		ret[connection.target.toNeuronRowOnDLS().toHemisphereOnDLS()][connection.receptor_type]
		    .insert(
		        {std::get<0>(connection.source_descriptor),
		         std::get<1>(connection.source_descriptor)});
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

	auto const anchors = get_anchors();

	std::set<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> neurons;
	using namespace halco::hicann_dls::vx::v3;
	for (auto const& partitioned_vertex_descriptor : m_partitioned_vertex_descriptors) {
		if (auto const* recorder = dynamic_cast<grenade::common::Recorder const*>(
		        &m_topology.get_reference().get(partitioned_vertex_descriptor));
		    recorder) {
			for (auto const& in_edge_descriptor :
			     m_topology.get_reference().in_edges(partitioned_vertex_descriptor)) {
				auto const source_vertex_descriptor =
				    m_topology.get_reference().source(in_edge_descriptor);
				auto const& population = dynamic_cast<grenade::common::Population const&>(
				    m_topology.get_reference().get(source_vertex_descriptor));
				if (auto const* locally_placed_neuron =
				        dynamic_cast<abstract::LocallyPlacedNeuron const*>(&population.get_cell());
				    locally_placed_neuron) {
					auto const& in_edge = m_topology.get_reference().get(in_edge_descriptor);
					auto const source_channels = in_edge.get_channels_on_source().get_elements();
					auto const population_section_elements = population.get_shape().get_elements();
					std::set<size_t> cell_on_population_dimensions;
					for (size_t i = 0; i < population.get_shape().dimensionality(); ++i) {
						cell_on_population_dimensions.insert(i);
					}
					auto const source_neuron_channels =
					    in_edge.get_channels_on_source()
					        .projection(cell_on_population_dimensions)
					        ->get_elements();
					for (size_t i = 0; i < source_channels.size(); ++i) {
						size_t const cell_on_population = std::distance(
						    population_section_elements.begin(),
						    std::find(
						        population_section_elements.begin(),
						        population_section_elements.end(), source_neuron_channels.at(i)));
						grenade::common::CompartmentOnNeuron const compartment_on_neuron(
						    source_channels.at(i).value.at(
						        population.get_shape().dimensionality()));
						auto const& spike_master =
						    locally_placed_neuron->compartments.at(compartment_on_neuron)
						        .spike_master;
						assert(spike_master);
						neurons.insert(
						    LogicalNeuronOnDLS(
						        locally_placed_neuron->shape,
						        anchors.at(source_vertex_descriptor).at(cell_on_population))
						        .get_placed_compartments()
						        .at(CompartmentOnLogicalNeuron(compartment_on_neuron))
						        .at(spike_master->atomic_neuron_on_compartment));
					}
				}
			}
		}
		if (auto const* population = dynamic_cast<grenade::common::Population const*>(
		        &m_topology.get_reference().get(partitioned_vertex_descriptor));
		    population) {
			if (auto const* locally_placed_neuron =
			        dynamic_cast<abstract::LocallyPlacedNeuron const*>(&population->get_cell());
			    locally_placed_neuron) {
				for (auto const& out_edge_descriptor :
				     m_topology.get_reference().out_edges(partitioned_vertex_descriptor)) {
					auto const target_vertex_descriptor =
					    m_topology.get_reference().target(out_edge_descriptor);
					if (std::find(
					        m_partitioned_vertex_descriptors.begin(),
					        m_partitioned_vertex_descriptors.end(),
					        target_vertex_descriptor) != m_partitioned_vertex_descriptors.end()) {
						// not on other execution instance
						continue;
					}
					auto const& out_edge = m_topology.get_reference().get(out_edge_descriptor);
					if (out_edge.port_on_source != 0) {
						// no spikes
						continue;
					}
					auto const source_channels = out_edge.get_channels_on_source().get_elements();
					auto const population_section_elements = population->get_shape().get_elements();
					std::set<size_t> cell_on_population_dimensions;
					for (size_t i = 0; i < population->get_shape().dimensionality(); ++i) {
						cell_on_population_dimensions.insert(i);
					}
					auto const source_neuron_channels =
					    out_edge.get_channels_on_source()
					        .projection(cell_on_population_dimensions)
					        ->get_elements();
					for (size_t i = 0; i < source_channels.size(); ++i) {
						size_t const cell_on_population = std::distance(
						    population_section_elements.begin(),
						    std::find(
						        population_section_elements.begin(),
						        population_section_elements.end(), source_neuron_channels.at(i)));
						grenade::common::CompartmentOnNeuron const compartment_on_neuron(
						    source_channels.at(i).value.at(
						        population->get_shape().dimensionality()));
						auto const& spike_master =
						    locally_placed_neuron->compartments.at(compartment_on_neuron)
						        .spike_master;
						assert(spike_master);
						neurons.insert(
						    LogicalNeuronOnDLS(
						        locally_placed_neuron->shape,
						        anchors.at(partitioned_vertex_descriptor).at(cell_on_population))
						        .get_placed_compartments()
						        .at(CompartmentOnLogicalNeuron(compartment_on_neuron))
						        .at(spike_master->atomic_neuron_on_compartment));
					}
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

	// get all atomic neurons by iterating partitioned and mapped vertices
	for (auto const& partitioned_vertex_descriptor : m_partitioned_vertex_descriptors) {
		for (auto const& inter_graph_hyper_edge_descriptor :
		     m_topology.inter_graph_hyper_edges_by_reference(partitioned_vertex_descriptor)) {
			auto const& inter_graph_hyper_edge = m_topology.get(inter_graph_hyper_edge_descriptor);
			if (auto const locally_placed_neuron_mapping =
			        dynamic_cast<abstract::LocallyPlacedNeuronMapping const*>(
			            &inter_graph_hyper_edge);
			    locally_placed_neuron_mapping) {
				for (auto const& mapped_vertex_descriptor :
				     m_topology.links(inter_graph_hyper_edge_descriptor)) {
					for (auto const& inter_graph_hyper_edge_descriptor :
					     m_topology.inter_graph_hyper_edges_by_linked(mapped_vertex_descriptor)) {
						auto const& partitioned_vertex_descriptor =
						    m_topology.references(inter_graph_hyper_edge_descriptor).at(0);
						if (std::find(
						        m_partitioned_vertex_descriptors.begin(),
						        m_partitioned_vertex_descriptors.end(),
						        partitioned_vertex_descriptor) !=
						    m_partitioned_vertex_descriptors.end()) {
							auto const& neurons =
							    dynamic_cast<signal_flow::vertex::NeuronView const&>(
							        m_topology.get(mapped_vertex_descriptor));
							for (auto const& column : neurons.get_columns()) {
								ret.insert(halco::hicann_dls::vx::v3::AtomicNeuronOnDLS(
								    column, neurons.row));
							}
						}
					}
				}
			}
		}
	}

	// remove all neurons present in `get_neurons_on_event_output()`
	auto const neurons_on_event_output = get_neurons_on_event_output();
	for (auto const& [_, neurons] : neurons_on_event_output) {
		for (auto const& neuron : neurons) {
			ret.erase(neuron);
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

	for (auto const& partitioned_vertex_descriptor : m_partitioned_vertex_descriptors) {
		for (auto const& inter_graph_hyper_edge_descriptor :
		     m_topology.inter_graph_hyper_edges_by_reference(partitioned_vertex_descriptor)) {
			auto const& links = m_topology.links(inter_graph_hyper_edge_descriptor);
			if (!links.empty()) {
				auto const& mapped_vertex_descriptor = links.at(0);
				if (auto const background_spike_source =
				        dynamic_cast<signal_flow::vertex::BackgroundSpikeSource const*>(
				            &m_topology.get(mapped_vertex_descriptor));
				    background_spike_source) {
					auto const population = dynamic_cast<grenade::common::Population const&>(
					    m_topology.get_reference().get(partitioned_vertex_descriptor));
					ret[background_spike_source->coordinate.toPADIBusOnDLS()] +=
					    population.get_shape().size();
				}
			}
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

std::map<
    grenade::common::VertexOnTopology,
    std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>>
RoutingConstraints::get_anchors() const
{
	std::map<
	    grenade::common::VertexOnTopology,
	    std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>>
	    ret;
	for (auto const& inter_graph_hyper_edge_descriptor : m_topology.inter_graph_hyper_edges()) {
		if (auto const& locally_placed_mapping =
		        dynamic_cast<abstract::LocallyPlacedNeuronMapping const*>(
		            &m_topology.get(inter_graph_hyper_edge_descriptor));
		    locally_placed_mapping) {
			auto& local_anchors =
			    ret[m_topology.references(inter_graph_hyper_edge_descriptor).at(0)];
			local_anchors.resize(
			    dynamic_cast<grenade::common::Population const&>(
			        m_topology.get_reference().get(
			            m_topology.references(inter_graph_hyper_edge_descriptor).at(0)))
			        .get_shape()
			        .size());
			for (auto const& [cell_on_population, la] : locally_placed_mapping->anchors) {
				auto const& [_, local_anchor] = la;
				local_anchors.at(cell_on_population) = local_anchor;
			}
		}
	}
	return ret;
}

std::map<
    std::tuple<
        grenade::common::VertexOnTopology,
        size_t,
        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>
RoutingConstraints::get_internal_sources() const
{
	std::map<
	    std::tuple<
	        grenade::common::VertexOnTopology, size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
	    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>
	    ret;

	auto const anchors = get_anchors();

	std::set<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> neurons;
	using namespace halco::hicann_dls::vx::v3;
	for (auto const& partitioned_vertex_descriptor : m_partitioned_vertex_descriptors) {
		if (auto const* population = dynamic_cast<grenade::common::Population const*>(
		        &m_topology.get_reference().get(partitioned_vertex_descriptor));
		    population) {
			if (auto const* locally_placed_neuron =
			        dynamic_cast<abstract::LocallyPlacedNeuron const*>(&population->get_cell());
			    locally_placed_neuron) {
				for (size_t cell_on_population = 0; cell_on_population < population->size();
				     ++cell_on_population) {
					for (auto const& [compartment_on_neuron, compartment] :
					     locally_placed_neuron->compartments) {
						auto const& spike_master = compartment.spike_master;
						if (spike_master) {
							ret.emplace(
							    std::make_tuple(
							        partitioned_vertex_descriptor, cell_on_population,
							        CompartmentOnLogicalNeuron(compartment_on_neuron)),
							    LogicalNeuronOnDLS(
							        locally_placed_neuron->shape,
							        anchors.at(partitioned_vertex_descriptor)
							            .at(cell_on_population))
							        .get_placed_compartments()
							        .at(CompartmentOnLogicalNeuron(compartment_on_neuron))
							        .at(spike_master->atomic_neuron_on_compartment));
						}
					}
				}
			}
		}
	}

	return ret;
}

} // namespace grenade::vx::network::routing::greedy
