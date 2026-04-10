#pragma once

#include "grenade/common/detail/graph.h"
#include "grenade/common/edge.h"
#include "grenade/common/edge_on_topology.h"
#include "grenade/common/graph.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/routing/csp/routing_vertex.h"

namespace grenade::vx::network::routing::csp {

// forward declaration
struct RoutingGraph;

} // namespace grenade::vx::network::routing::csp
namespace grenade::common {

// Make Graph visible for linker
extern template class SYMBOL_VISIBLE Graph<
    grenade::vx::network::routing::csp::RoutingGraph,
    detail::BidirectionalGraph,
    grenade::vx::network::routing::csp::RoutingVertex,
    Edge,
    VertexOnTopology,
    EdgeOnTopology,
    std::shared_ptr>;

} // namespace grenade::common
namespace grenade::vx::network::routing::csp {

/**
 * Routing graph.
 */
struct SYMBOL_VISIBLE RoutingGraph
    : public grenade::common::Graph<
          RoutingGraph,
          grenade::common::detail::BidirectionalGraph,
          RoutingVertex,
          grenade::common::Edge,
          grenade::common::VertexOnTopology,
          grenade::common::EdgeOnTopology,
          std::shared_ptr>
{
	/**
	 * Update parameters in graph for given CSP-solver space.
	 * This method is to be called after copying into a new space.
	 * @param space CSP-solver space to update for
	 * @param other Graph to update from
	 */
	void update(Gecode::Space& space, RoutingGraph& other);

	/**
	 * Get parameters in graph, e.g. for branching.
	 * @return Parameters of graph entities
	 */
	Gecode::IntVarArgs get_parameters() const;

	/**
	 * Add tracing for vertex properties.
	 */
	void trace(Gecode::Space& space);
};

} // namespace grenade::vx::network::routing::csp
