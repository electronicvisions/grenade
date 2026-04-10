#pragma once
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/routing/csp/route_filter_sequence.h"
#include "grenade/vx/network/routing/csp/routing_graph.h"
#include "grenade/vx/network/routing/csp/source_target_pair.h"
#include "hate/visibility.h"
#include <map>
#include <set>
#include <vector>

namespace grenade::vx::network::routing::csp {

/**
 * Route collector which finds all routes from source nodes to target nodes on a routing graph.
 */
struct SYMBOL_VISIBLE RouteCollector
{
	/**
	 * Routes per pair of source and target vertex.
	 * A full route consists of its source, its target and the filter sequence between them.
	 */
	typedef std::map<SourceTargetPair, std::vector<RouteFilterSequence>> Routes;


	RouteCollector() = default;

	/**
	 * Collect routes from routing graph.
	 * @param graph Routing graph to collect routes for
	 * @returns Pair of acyclic and cyclic routes.
	 */
	std::pair<Routes, Routes> operator()(RoutingGraph const& routing_graph) SYMBOL_VISIBLE;

private:
	void recursive_route_collection(
	    grenade::common::VertexOnTopology vertex,
	    RoutingGraph const& routing_graph,
	    Routes& routes_acyclic,
	    Routes& routes_cyclic,
	    std::vector<grenade::common::VertexOnTopology>& current_route,
	    std::set<grenade::common::VertexOnTopology> current_route_set);
};

std::ostream& operator<<(std::ostream& os, SourceTargetPair const& value);

} // namespace grenade::vx::network::routing::csp
