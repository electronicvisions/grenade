#include "grenade/vx/network/abstract/topology_rewrite/uncalibrated_signed_synapse.h"

#include "grenade/common/edge_on_topology.h"
#include "grenade/common/inter_topology_hyper_edge/projection.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence_dimension_unit/receptor_on_compartment.h"
#include "grenade/common/projection.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/mapping/uncalibrated_signed_synapse.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated_signed.h"
#include "hate/math.h"
#include <memory>
#include <stdexcept>

namespace grenade::vx::network::abstract {

UncalibratedSignedSynapseRewrite::UncalibratedSignedSynapseRewrite(
    std::shared_ptr<grenade::common::LinkedTopology> topology) :
    TopologyRewrite(std::move(topology))
{
}

void UncalibratedSignedSynapseRewrite::operator()() const
{
	using namespace grenade::common;
	// copy vertex descriptors because they are modified in the loop (by replacing projections)
	std::vector<VertexOnTopology> all_projection_vertices;
	for (auto const& vertex_descriptor : get_topology().vertices()) {
		if (auto const ptr =
		        dynamic_cast<Projection const*>(&(get_topology().get(vertex_descriptor)));
		    ptr) {
			if (auto const synapse_ptr =
			        dynamic_cast<UncalibratedSignedSynapse const*>(&ptr->get_synapse());
			    synapse_ptr) {
				all_projection_vertices.push_back(vertex_descriptor);
			}
		}
	}

	for (auto const vertex_descriptor : all_projection_vertices) {
		auto const& old_projection =
		    dynamic_cast<Projection const&>(get_topology().get(vertex_descriptor));

		// get reference projection vertex descriptor
		std::unordered_set<typename LinkedTopology::ReferenceGraph::VertexDescriptor>
		    old_references;
		for (auto const& link :
		     get_topology().inter_graph_hyper_edges_by_linked(vertex_descriptor)) {
			auto const& references = get_topology().references(link);
			assert(references.size() == 1);
			old_references.insert(references.at(0));
		}
		assert(old_references.size() == 1);
		auto const& old_reference = *old_references.begin();

		auto const& old_parameter_space =
		    dynamic_cast<UncalibratedSignedSynapse::ParameterSpace const&>(
		        old_projection.get_synapse_parameter_space());
		auto const& old_synapse =
		    dynamic_cast<UncalibratedSignedSynapse const&>(old_projection.get_synapse());
		std::vector<std::tuple<VertexOnTopology, Edge>> new_in_edges;
		std::vector<std::tuple<VertexOnTopology, Edge>> new_exc_out_edges;
		std::vector<std::tuple<VertexOnTopology, Edge>> new_inh_out_edges;
		std::vector<VertexOnTopology> new_projection_descriptors;
		for (auto const in_edge : get_topology().in_edges(vertex_descriptor)) {
			auto const source_descriptor = get_topology().source(in_edge);
			new_in_edges.push_back(std::make_tuple(source_descriptor, get_topology().get(in_edge)));
		}

		std::set<size_t> projection_dimensions_except_receptor;
		for (size_t i = 0;
		     i < old_projection.get_output_ports().at(0).get_channels().dimensionality() - 1; ++i) {
			projection_dimensions_except_receptor.insert(i);
		}

		for (auto const out_edge : get_topology().out_edges(vertex_descriptor)) {
			auto const target_descriptor = get_topology().target(out_edge);
			auto const& old_edge = get_topology().get(out_edge);

			if (old_edge.port_on_source != 0) {
				throw std::runtime_error("Rewriting signed synapse projection with edges using "
				                         "port != 0 not supported.");
			}

			auto const exc_source_channels_space =
			    old_edge.get_channels_on_source()
			        .distinct_projection(projection_dimensions_except_receptor)
			        ->cartesian_product(grenade::common::CuboidMultiIndexSequence(
			            {1}, {grenade::common::MultiIndex({old_synapse.excitatory_receptor})},
			            {grenade::common::ReceptorOnCompartmentDimensionUnit()}));

			auto const inh_source_channels_space =
			    old_edge.get_channels_on_source()
			        .distinct_projection(projection_dimensions_except_receptor)
			        ->cartesian_product(grenade::common::CuboidMultiIndexSequence(
			            {1}, {grenade::common::MultiIndex({old_synapse.inhibitory_receptor})},
			            {grenade::common::ReceptorOnCompartmentDimensionUnit()}));

			// we use distinct_projection here, since we know, that the result will be distinct
			// (since synapses can't be targeted by more than one source)
			auto const exc_source_channels =
			    old_edge.get_channels_on_source()
			        .subset_restriction(*exc_source_channels_space)
			        ->distinct_projection(projection_dimensions_except_receptor);

			// we use distinct_projection here, since we know, that the result will be distinct
			// (since synapses can't target more than one target)
			auto const inh_source_channels =
			    old_edge.get_channels_on_source()
			        .subset_restriction(*inh_source_channels_space)
			        ->distinct_projection(projection_dimensions_except_receptor);

			auto const exc_target_channels =
			    old_edge.get_channels_on_target().related_sequence_subset_restriction(
			        old_edge.get_channels_on_source(), *exc_source_channels_space);
			auto const inh_target_channels =
			    old_edge.get_channels_on_target().related_sequence_subset_restriction(
			        old_edge.get_channels_on_source(), *inh_source_channels_space);

			Edge new_exc_edge(
			    *exc_source_channels, *exc_target_channels, old_edge.port_on_source,
			    old_edge.port_on_target);
			Edge new_inh_edge(
			    *inh_source_channels, *inh_target_channels, old_edge.port_on_source,
			    old_edge.port_on_target);

			new_exc_out_edges.push_back(std::make_tuple(target_descriptor, new_exc_edge));
			new_inh_out_edges.push_back(std::make_tuple(target_descriptor, new_inh_edge));
		}

		std::vector<UncalibratedSynapse::Weight> new_exc_max_weights(old_parameter_space.size());
		std::vector<UncalibratedSynapse::Weight> new_inh_max_weights(old_parameter_space.size());
		for (size_t i = 0; i < old_parameter_space.size(); ++i) {
			new_exc_max_weights.at(i) = UncalibratedSynapse::Weight(std::max(
			    UncalibratedSignedSynapse::Weight(0), old_parameter_space.max_weights.at(i)));
			new_inh_max_weights.at(i) = UncalibratedSynapse::Weight(std::max(
			    UncalibratedSignedSynapse::Weight(0), old_parameter_space.max_weights.at(i)));
		}
		UncalibratedSynapse::ParameterSpace new_exc_parameter_space(std::move(new_exc_max_weights));
		UncalibratedSynapse::ParameterSpace new_inh_parameter_space(std::move(new_inh_max_weights));

		Projection new_exc_projection(
		    UncalibratedSynapse(), new_exc_parameter_space, old_projection.get_connector(),
		    old_projection.get_time_domain(), old_projection.get_execution_instance_on_executor());
		Projection new_inh_projection(
		    UncalibratedSynapse(), new_inh_parameter_space, old_projection.get_connector(),
		    old_projection.get_time_domain(), old_projection.get_execution_instance_on_executor());

		auto const new_exc_projection_descriptor = get_topology().add_vertex(new_exc_projection);
		auto const new_inh_projection_descriptor = get_topology().add_vertex(new_inh_projection);

		new_projection_descriptors.push_back(new_exc_projection_descriptor);
		new_projection_descriptors.push_back(new_inh_projection_descriptor);

		get_topology().clear_vertex(vertex_descriptor);

		// set links to new sections
		get_topology().add_inter_graph_hyper_edge(
		    {new_exc_projection_descriptor, new_inh_projection_descriptor}, {old_reference},
		    UncalibratedSignedSynapseMapping());

		// add in-edges to new sections
		for (auto const& [source, edge] : new_in_edges) {
			for (auto const& target : new_projection_descriptors) {
				get_topology().add_edge(source, target, edge);
			}
		}

		// add out-edges to new sections
		for (auto const& [target, edge] : new_exc_out_edges) {
			get_topology().add_edge(new_exc_projection_descriptor, target, edge);
		}
		for (auto const& [target, edge] : new_inh_out_edges) {
			get_topology().add_edge(new_inh_projection_descriptor, target, edge);
		}

		get_topology().remove_vertex(vertex_descriptor);
	}
}

} // namespace grenade::vx::network::abstract
