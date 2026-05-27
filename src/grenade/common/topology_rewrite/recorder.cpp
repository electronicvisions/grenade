#include "grenade/common/topology_rewrite/recorder.h"

#include "grenade/common/edge_on_topology.h"
#include "grenade/common/inter_topology_hyper_edge/recorder.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/population.h"
#include "grenade/common/recorder.h"
#include "grenade/common/vertex_on_topology.h"
#include "hate/math.h"
#include <memory>
#include <stdexcept>

namespace grenade::common {

RecorderTopologyRewrite::RecorderTopologyRewrite(std::shared_ptr<LinkedTopology> topology) :
    TopologyRewrite(std::move(topology))
{
}

void RecorderTopologyRewrite::operator()() const
{
	// copy vertex descriptors because they are modified in the loop (by replacing recorders)
	std::vector<VertexOnTopology> all_recorder_vertices;
	for (auto const& vertex_descriptor : get_topology().vertices()) {
		if (auto const ptr =
		        dynamic_cast<Recorder const*>(&(get_topology().get(vertex_descriptor)));
		    ptr) {
			all_recorder_vertices.push_back(vertex_descriptor);
		}
	}
	for (auto const vertex_descriptor : all_recorder_vertices) {
		// get reference recorder vertex descriptor
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

		std::vector<std::tuple<VertexOnTopology, VertexOnTopology, Edge>> new_in_edges;
		std::vector<std::tuple<VertexOnTopology, Edge>> new_out_edges;
		std::vector<VertexOnTopology> new_vertices;
		auto const& recorder = dynamic_cast<Recorder const&>(get_topology().get(vertex_descriptor));
		// split recorder along edges from sources
		for (auto const in_edge : get_topology().in_edges(vertex_descriptor)) {
			auto const source_descriptor = get_topology().source(in_edge);
			if (auto const ptr =
			        dynamic_cast<Population const*>(&get_topology().get(source_descriptor));
			    ptr) {
				auto const& channels_on_recorder =
				    get_topology().get(in_edge).get_channels_on_target();
				auto new_recorder = recorder.get_section(channels_on_recorder);
				assert(new_recorder);
				// use same execution instance as population
				auto& new_recorder_partitioned_vertex =
				    dynamic_cast<PartitionedVertex&>(*new_recorder);
				new_recorder_partitioned_vertex.set_execution_instance_on_executor(
				    ptr->get_execution_instance_on_executor());
				auto const new_recorder_descriptor =
				    get_topology().add_vertex(std::move(*new_recorder));
				auto const& channels_on_population =
				    get_topology().get(in_edge).get_channels_on_source();
				new_in_edges.push_back(std::make_tuple(
				    source_descriptor, new_recorder_descriptor,
				    Edge(
				        channels_on_population, channels_on_recorder,
				        get_topology().get(in_edge).port_on_source,
				        get_topology().get(in_edge).port_on_target)));
				new_vertices.push_back(new_recorder_descriptor);
			} else {
				throw std::runtime_error(
				    "Sources other than Population not supported for Recorder rewrite.");
			}
		}

		// create new edges to all targets
		for (auto const& new_vertex_descriptor : new_vertices) {
			auto const& new_vertex =
			    dynamic_cast<Recorder const&>(get_topology().get(new_vertex_descriptor));
			for (auto const out_edge_descriptor : get_topology().out_edges(vertex_descriptor)) {
				auto const target_descriptor = get_topology().target(out_edge_descriptor);
				auto const& out_edge = get_topology().get(out_edge_descriptor);
				// logic below only works for injective edge channels
				if (!out_edge.get_channels_on_source().is_injective() ||
				    !out_edge.get_channels_on_target().is_injective()) {
					throw std::runtime_error("RecorderTopologyRewrite edge rewrite only works "
					                         "for injective out_edge channels");
				}
				auto const channels_on_recorder =
				    new_vertex.get_output_ports()
				        .at(out_edge.port_on_source)
				        .get_channels()
				        .subset_restriction(out_edge.get_channels_on_source());
				assert(channels_on_recorder);
				auto const channels_on_target =
				    out_edge.get_channels_on_target().related_sequence_subset_restriction(
				        out_edge.get_channels_on_source(),
				        new_vertex.get_output_ports().at(out_edge.port_on_source).get_channels());
				assert(channels_on_target);
				if (channels_on_recorder->size() == 0) {
					continue;
				}
				new_out_edges.push_back(std::make_tuple(
				    target_descriptor, Edge(
				                           *channels_on_recorder, *channels_on_target,
				                           out_edge.port_on_source, out_edge.port_on_target)));
			}
		}

		get_topology().clear_vertex(vertex_descriptor);

		// set links to new recorders
		get_topology().add_inter_graph_hyper_edge(
		    new_vertices, {old_reference}, RecorderInterTopologyHyperEdge());

		// add in-edges to new recorders
		for (auto const& [source, target, edge] : new_in_edges) {
			get_topology().add_edge(source, target, edge);
		}

		// add out-edges to new recorders
		for (auto const& [target, edge] : new_out_edges) {
			for (auto const& source : new_vertices) {
				get_topology().add_edge(source, target, edge);
			}
		}

		get_topology().remove_vertex(vertex_descriptor);
	}
}

} // namespace grenade::common
