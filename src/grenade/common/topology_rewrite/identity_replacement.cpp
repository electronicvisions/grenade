#include "grenade/common/topology_rewrite/identity_replacement.h"

#include "grenade/common/edge_on_topology.h"
#include "grenade/common/inter_topology_hyper_edge/identity.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/vertex_on_topology.h"
#include <map>

namespace grenade::common {

IdentityReplacementTopologyRewrite::IdentityReplacementTopologyRewrite(
    std::shared_ptr<LinkedTopology> topology) :
    TopologyRewrite(std::move(topology))
{
}

void IdentityReplacementTopologyRewrite::operator()() const
{
	auto& topology = get_topology();

	std::map<VertexOnTopology, VertexOnTopology> linked_vertices;
	for (auto const vertex : topology.get_reference().vertices()) {
		auto copy = topology.get_reference().get(vertex).copy();
		assert(copy);
		linked_vertices.emplace(vertex, topology.add_vertex(std::move(*copy)));
	}

	for (auto const edge_descriptor : topology.get_reference().edges()) {
		topology.add_edge(
		    linked_vertices.at(topology.get_reference().source(edge_descriptor)),
		    linked_vertices.at(topology.get_reference().target(edge_descriptor)),
		    topology.get_reference().get(edge_descriptor));
	}

	for (auto const& [reference, linked] : linked_vertices) {
		topology.add_inter_graph_hyper_edge(
		    {linked}, {reference}, IdentityInterTopologyHyperEdge());
	}
}

} // namespace grenade::common
