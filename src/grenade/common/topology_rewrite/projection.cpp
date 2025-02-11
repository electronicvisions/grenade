#include "grenade/common/topology_rewrite/projection.h"

#include "grenade/common/edge_on_topology.h"
#include "grenade/common/inter_topology_hyper_edge/projection.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/population.h"
#include "grenade/common/projection.h"
#include "grenade/common/vertex_on_topology.h"
#include "hate/math.h"
#include <memory>
#include <stdexcept>

namespace grenade::common {

ProjectionTopologyRewrite::ProjectionTopologyRewrite(std::shared_ptr<LinkedTopology> topology) :
    TopologyRewrite(std::move(topology))
{
}

void ProjectionTopologyRewrite::operator()() const
{
	// copy vertex descriptors because they are modified in the loop (by replacing projections)
	std::vector<VertexOnTopology> all_projection_vertices;
	for (auto const& vertex_descriptor : get_topology().vertices()) {
		if (auto const ptr =
		        dynamic_cast<Projection const*>(&(get_topology().get(vertex_descriptor)));
		    ptr) {
			all_projection_vertices.push_back(vertex_descriptor);
		}
	}
	for (auto const vertex_descriptor : all_projection_vertices) {
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

		std::vector<std::tuple<VertexOnTopology, VertexOnTopology, Edge>> new_edges;
		std::vector<std::tuple<VertexOnTopology, Edge>> new_in_edges;
		std::vector<VertexOnTopology> new_vertices;
		for (auto const in_edge : get_topology().in_edges(vertex_descriptor)) {
			auto const source_descriptor = get_topology().source(in_edge);
			new_in_edges.push_back(std::make_tuple(source_descriptor, get_topology().get(in_edge)));
		}

		auto const& projection =
		    dynamic_cast<Projection const&>(get_topology().get(vertex_descriptor));
		auto const connector_input_sequence = projection.get_connector().get_input_sequence();
		auto const connector_output_sequence = projection.get_connector().get_output_sequence();
		auto const connector_io_sequence =
		    connector_input_sequence->cartesian_product(*connector_output_sequence);
		// split projection along edges to post-synaptic populations
		for (auto const out_edge : get_topology().out_edges(vertex_descriptor)) {
			auto const target_descriptor = get_topology().target(out_edge);
			if (auto const ptr =
			        dynamic_cast<Population const*>(&get_topology().get(target_descriptor));
			    ptr) {
				auto const& channels_on_projection =
				    get_topology().get(out_edge).get_channels_on_source();

				std::set<size_t> projection_dimensions;
				for (size_t i = 0;
				     i < projection.get_connector().get_input_sequence()->dimensionality() -
				             projection.get_synapse()
				                 .get_input_ports()
				                 .projection.at(get_topology().get(out_edge).port_on_source)
				                 .get_channels()
				                 .dimensionality();
				     ++i) {
					projection_dimensions.insert(i);
				}

				auto const projection_channels_on_source =
				    channels_on_projection.distinct_projection(projection_dimensions);
				auto const projection_sequence_slice =
				    projection.get_connector().get_input_sequence()->cartesian_product(
				        *projection_channels_on_source);
				assert(projection_sequence_slice);
				auto const projection_sequence =
				    projection_sequence_slice->subset_restriction(*connector_io_sequence);
				assert(projection_sequence);
				auto projection_connector =
				    projection.get_connector().get_section(*projection_sequence);
				auto projection_parameter_space =
				    projection.get_synapse_parameter_space().get_section(
				        *CuboidMultiIndexSequence({projection.get_synapse_parameter_space().size()})
				             .subset_restriction(
				                 *projection_connector->get_synapse_parameterization_indices(
				                     *projection_sequence)));

				Projection new_projection(
				    projection.get_synapse(), *projection_parameter_space, *projection_connector,
				    projection.get_time_domain(), projection.get_execution_instance_on_executor());
				auto const new_projection_descriptor = get_topology().add_vertex(new_projection);

				auto const& channels_on_target =
				    get_topology().get(out_edge).get_channels_on_target();
				new_edges.push_back(std::make_tuple(
				    new_projection_descriptor, target_descriptor,
				    Edge(
				        channels_on_projection, channels_on_target,
				        get_topology().get(out_edge).port_on_source,
				        get_topology().get(out_edge).port_on_target)));
				new_vertices.push_back(new_projection_descriptor);
			}
		}
		// create new edges to all vertices which are not populations
		for (auto const& new_vertex_descriptor : new_vertices) {
			auto const& new_vertex =
			    dynamic_cast<Projection const&>(get_topology().get(new_vertex_descriptor));
			auto const new_vertex_output_ports = new_vertex.get_output_ports();
			for (auto const out_edge_descriptor : get_topology().out_edges(vertex_descriptor)) {
				auto const target_descriptor = get_topology().target(out_edge_descriptor);
				if (auto const ptr =
				        dynamic_cast<Population const*>(&get_topology().get(target_descriptor));
				    !ptr) {
					auto const& out_edge = get_topology().get(out_edge_descriptor);

					auto const channels_on_source =
					    new_vertex_output_ports.at(out_edge.port_on_source)
					        .get_channels()
					        .subset_restriction(out_edge.get_channels_on_source());
					assert(channels_on_source);

					auto const channels_on_target =
					    out_edge.get_channels_on_target().related_sequence_subset_restriction(
					        out_edge.get_channels_on_source(),
					        new_vertex_output_ports.at(out_edge.port_on_source).get_channels());
					assert(channels_on_target);

					if (channels_on_source->size() == 0) {
						continue;
					}

					new_edges.push_back(std::make_tuple(
					    new_vertex_descriptor, get_topology().target(out_edge_descriptor),
					    Edge(
					        *channels_on_source, *channels_on_target, out_edge.port_on_source,
					        out_edge.port_on_target)));
				}
			}
		}

		get_topology().clear_vertex(vertex_descriptor);

		// set links to new projections
		for (auto const& new_vertex : new_vertices) {
			get_topology().add_inter_graph_hyper_edge(
			    {new_vertex}, {old_reference}, ProjectionInterTopologyHyperEdge());
		}

		// add in-edges to new projections
		for (auto const& [source, edge] : new_in_edges) {
			for (auto const& target : new_vertices) {
				get_topology().add_edge(source, target, edge);
			}
		}

		// add out-edges to new projections
		for (auto const& [source, target, edge] : new_edges) {
			get_topology().add_edge(source, target, edge);
		}

		get_topology().remove_vertex(vertex_descriptor);
	}
}

} // namespace grenade::common
