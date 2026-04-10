#pragma once
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/routing/csp/route_filter_sequence.h"
#include "grenade/vx/network/routing/csp/router_options.h"
#include "grenade/vx/network/routing/csp/router_space_shared_storage.h"
#include "grenade/vx/network/routing/csp/routing_graph.h"
#include "grenade/vx/network/routing/csp/source_target_pair.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <map>
#include <memory>
#include <vector>
#include <gecode/int.hh>
#include <gecode/kernel.hh>


namespace grenade::vx::network::routing::csp {

// Forward declaration
struct RouterSpaceFactory;
struct Router;

/**
 * Router space containing all parameters, constraints and branching directives used by CSP-solver.
 */
struct SYMBOL_VISIBLE RouterSpace : public Gecode::Space
{
	/**
	 * Construct router space for network.
	 * @param options Routing options
	 */
	RouterSpace() SYMBOL_VISIBLE;

	/**
	 * Copy construct from other router space instance.
	 * This is to be used only in the search algorithm and copies only what is needed for it to work
	 * but leaves the other state shared.
	 */
	RouterSpace(RouterSpace& other) SYMBOL_VISIBLE;

	/**
	 * Copy router space instance.
	 * This is to be used only in the search algorithm and copies only what is needed for it to work
	 * but leaves the other state shared.
	 */
	virtual Gecode::Space* copy() SYMBOL_VISIBLE;

	RoutingGraph const& get_graph() const;

	friend std::ostream& operator<<(std::ostream& os, RouterSpace const& router_space)
	    SYMBOL_VISIBLE;

private:
	friend struct RouterSpaceFactory;
	friend struct Router;

	/**
	 * Shared storage between copies of the router space during CSP search.
	 */
	std::shared_ptr<RouterSpaceSharedStorage> m_shared_storage;

	/**
	 * Graph of routing entities.
	 */
	RoutingGraph m_routing_graph;

	/**
	 * Boolean activity values of all possible routes per source-target-pair per source index on
	 * source vertex per route filter sequence (outer to inner dimension).
	 */
	std::map<SourceTargetPair, std::vector<Gecode::BoolVarArray>> m_route_activities;
};

} // namespace grenade::vx::network::routing::csp