#include "grenade/common/topology_rewrite/strong_component_invariant.h"

#include "grenade/common/edge.h"
#include "grenade/common/inter_topology_hyper_edge.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/strong_component_invariant_vertex.h"
#include "grenade/common/vertex_on_topology.h"
#include <functional>
#include <set>
#include <unordered_map>

namespace grenade::common {

StrongComponentInvariantRewrite::StrongComponentInvariantRewrite(
    std::shared_ptr<LinkedTopology> topology) :
    TopologyRewrite(std::move(topology))
{
}

void StrongComponentInvariantRewrite::operator()() const
{
	using namespace grenade::common;
	auto& topology = dynamic_cast<LinkedTopology&>(get_topology());

	std::unordered_map<
	    VertexOnTopology, std::reference_wrapper<StrongComponentInvariantVertex const>>
	    linked_vertices;

	std::unordered_map<VertexOnTopology, VertexOnTopology>
	    linked_descriptor_by_reference_descriptor;

	for (auto const& reference_vertex_descriptor : topology.get_reference().vertices()) {
		auto const& reference_vertex = topology.get_reference().get(reference_vertex_descriptor);
		auto strong_component_invariant = reference_vertex.get_strong_component_invariant();
		if (strong_component_invariant) {
			StrongComponentInvariantVertex strong_component_invariant_vertex(
			    std::move(*strong_component_invariant));
			auto strong_component_invariant_it = std::find_if(
			    linked_vertices.begin(), linked_vertices.end(),
			    [&](auto const& pair) { return pair.second == strong_component_invariant_vertex; });
			if (strong_component_invariant_it == linked_vertices.end()) {
				auto const linked_vertex_descriptor =
				    topology.add_vertex(std::move(strong_component_invariant_vertex));
				auto const& linked_vertex = static_cast<StrongComponentInvariantVertex const&>(
				    topology.get(linked_vertex_descriptor));
				strong_component_invariant_it =
				    linked_vertices.emplace(linked_vertex_descriptor, linked_vertex).first;
			}

			topology.add_inter_graph_hyper_edge(
			    {strong_component_invariant_it->first}, {reference_vertex_descriptor},
			    InterTopologyHyperEdge());

			linked_descriptor_by_reference_descriptor.emplace(
			    reference_vertex_descriptor, strong_component_invariant_it->first);
		}
	}

	std::set<std::pair<VertexOnTopology, VertexOnTopology>>
	    added_edges; // to only add each edge once
	for (auto const reference_edge_descriptor : topology.get_reference().edges()) {
		auto const& reference_source_descriptor =
		    topology.get_reference().source(reference_edge_descriptor);
		auto const& reference_target_descriptor =
		    topology.get_reference().target(reference_edge_descriptor);
		auto const& linked_source_descriptor =
		    linked_descriptor_by_reference_descriptor.at(reference_source_descriptor);
		auto const& linked_target_descriptor =
		    linked_descriptor_by_reference_descriptor.at(reference_target_descriptor);
		if (linked_source_descriptor != linked_target_descriptor) {
			if (added_edges.contains({linked_source_descriptor, linked_target_descriptor})) {
				continue;
			}
			added_edges.emplace(linked_source_descriptor, linked_target_descriptor);

			topology.add_edge(
			    linked_source_descriptor, linked_target_descriptor,
			    Edge(
			        grenade::common::CuboidMultiIndexSequence({1}),
			        grenade::common::CuboidMultiIndexSequence({1})));
		}
	}
}

} // namespace grenade::vx::execution::detail
