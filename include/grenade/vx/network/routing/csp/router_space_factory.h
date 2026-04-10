#pragma once

#include "grenade/vx/network/routing/csp/router_space.h"
#include "grenade/vx/network/routing/csp/routing_graph.h"
#include "hate/visibility.h"

namespace grenade::vx::network::routing::csp {

/**
 * Router space factory.
 */
struct SYMBOL_VISIBLE RouterSpaceFactory
{
	RouterSpaceFactory();

	/**
	 * Construct router space and constrain it based on given active routes, the routing graphs
	 * topology and routing options.
	 * The factory is reset and can be used to construct another routing space from scratch.
	 */
	std::unique_ptr<RouterSpace> done();

	/**
	 * Constrain the contained router space.
	 * This needs to be called before done() in order to have any constraints on the router space.
	 * @param active_routes Routes that need to be active on the contained router space.
	 */
	void build_constraints(std::map<SourceTargetPair, std::map<size_t, size_t>> active_routes);

protected:
	RouterSpace& get_space();
	RoutingGraph& get_graph();

	/**
	 * Construct all possible routes based on the routing graph.
	 */
	virtual std::pair<RouteCollector::Routes, RouteCollector::Routes> build_routes();

	/**
	 * Enforce distinct source and target labels.
	 * @param active_routes Routes that need to be active.
	 */
	virtual void build_label_constraints(
	    std::map<SourceTargetPair, std::map<size_t, size_t>> const& active_routes);

	/**
	 * @param routes_acyclic Acyclic routes on the contained router space.
	 * @param routes_cycic Cyclic routes on the contained router space.
	 * @param active_routes Routes that need to be active.
	 */
	virtual void build_route_constraints(
	    RouteCollector::Routes& routes_acyclic,
	    RouteCollector::Routes& routes_cyclic,
	    std::map<SourceTargetPair, std::map<size_t, size_t>> const& active_routes);

	/**
	 * Construct filter constraints.
	 * @param routes_acyclic Acyclic routes on the contained router space.
	 * @param routes_cycic Cyclic routes on the contained router space.
	 */
	virtual void build_filter_constraints(
	    RouteCollector::Routes& routes_acyclic, RouteCollector::Routes& routes_cyclic);

	/**
	 * Construct variable branching.
	 * @param routes_acyclic Acyclic routes on the contained router space.
	 * @param routes_cycic Cyclic routes on the contained router space.
	 */
	virtual void build_branching(
	    RouteCollector::Routes& routes_acyclic, RouteCollector::Routes& routes_cyclic);

	/**
	 * Construct variable tracing.
	 */
	virtual void build_tracing();

	std::unique_ptr<RouterSpace> m_router_space;
};

} // namespace grenade::vx::network::routing::csp