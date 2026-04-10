#include "grenade/vx/network/routing/csp/route_collector.h"
#include "grenade/vx/network/routing/csp/vertex/source.h"
#include "grenade/vx/network/routing/csp/vertex/target.h"

namespace grenade::vx::network::routing::csp {

std::pair<RouteCollector::Routes, RouteCollector::Routes> RouteCollector::operator()(
    RoutingGraph const& routing_graph)
{
	Routes acyclic_routes;
	Routes cyclic_routes;

	std::vector<grenade::common::VertexOnTopology> current_route;
	std::set<grenade::common::VertexOnTopology> current_route_set;

	for (auto const& vertex : routing_graph.vertices()) {
		if (dynamic_cast<Source const*>(&routing_graph.get(vertex))) {
			assert(current_route.size() == 0);
			assert(current_route_set.size() == 0);
			recursive_route_collection(
			    vertex, routing_graph, acyclic_routes, cyclic_routes, current_route,
			    current_route_set);
		}
	}
	return {acyclic_routes, cyclic_routes};
}

void RouteCollector::recursive_route_collection(
    grenade::common::VertexOnTopology vertex,
    RoutingGraph const& routing_graph,
    Routes& routes_acyclic,
    Routes& routes_cyclic,
    std::vector<grenade::common::VertexOnTopology>& current_route,
    std::set<grenade::common::VertexOnTopology> current_route_set)
{
	// Cyclic route
	if (current_route_set.contains(vertex)) {
		SourceTargetPair source_target_pair(current_route.front(), vertex);
		if (current_route.size() < 2) {
			throw std::logic_error("Route requires at least two elements.");
		}
		routes_cyclic[source_target_pair].emplace_back(
		    RouteFilterSequence::Descriptors(current_route.begin() + 1, current_route.end()));
		return;
	}

	current_route.push_back(vertex);
	current_route_set.insert(vertex);

	// Target reached
	if (dynamic_cast<Target const*>(&routing_graph.get(vertex))) {
		SourceTargetPair source_target_pair(current_route.front(), current_route.back());
		if (current_route.size() < 2) {
			throw std::logic_error("Route requires at least two elements.");
		}
		routes_acyclic[source_target_pair].emplace_back(
		    RouteFilterSequence::Descriptors(current_route.begin() + 1, current_route.end() - 1));
	}
	// Continue recursive search
	else {
		for (auto const& out_edge : routing_graph.out_edges(vertex)) {
			auto const target = routing_graph.target(out_edge);
			recursive_route_collection(
			    target, routing_graph, routes_acyclic, routes_cyclic, current_route,
			    current_route_set);
		}
	}

	current_route.pop_back();
	current_route_set.erase(vertex);
}

} // namespace grenade::vx::network::routing::csp