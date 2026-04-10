#include "grenade/vx/network/routing/csp/routing_graph.h"
#include "grenade/common/graph_impl.tcc"
#include "hate/indent.h"

namespace grenade::common {

template class Graph<
    grenade::vx::network::routing::csp::RoutingGraph,
    grenade::common::detail::BidirectionalGraph,
    grenade::vx::network::routing::csp::RoutingVertex,
    grenade::common::Edge,
    grenade::common::VertexOnTopology,
    grenade::common::EdgeOnTopology,
    std::shared_ptr>;

template std::ostream& operator<<(
    std::ostream&,
    Graph<
        grenade::vx::network::routing::csp::RoutingGraph,
        detail::BidirectionalGraph,
        grenade::vx::network::routing::csp::RoutingVertex,
        Edge,
        VertexOnTopology,
        EdgeOnTopology,
        std::shared_ptr> const&);

} // namespace grenade::common

namespace grenade::vx::network::routing::csp {

void RoutingGraph::update(Gecode::Space& space, RoutingGraph& other)
{
	if (num_vertices() != other.num_vertices()) {
		throw std::range_error("Different number of vertices on other graph.");
	}

	auto it1 = vertices().begin();
	auto it2 = other.vertices().begin();

	for (; it1 != vertices().end() && it2 != other.vertices().end(); ++it1, ++it2) {
		get_mutable(*it1).update(space, other.get_mutable(*it2));
	}
}

Gecode::IntVarArgs RoutingGraph::get_parameters() const
{
	Gecode::IntVarArgs ret;
	for (auto const& vertex : vertices()) {
		ret << get(vertex).get_parameters();
	}
	return ret;
}

void RoutingGraph::trace(Gecode::Space& space)
{
	for (auto const& vertex : vertices()) {
		get_mutable(vertex).trace(space);
	}
}

} // namespace grenade::vx::network::routing::csp