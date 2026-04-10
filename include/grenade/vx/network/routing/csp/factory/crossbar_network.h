#pragma once

#include "dapr/auto_key_map.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/routing/csp/router_space_factory.h"
#include "grenade/vx/network/routing/csp/routing_structures/crossbar.h"
#include "grenade/vx/network/routing/csp/routing_structures/crossbar_on_routing_graph.h"
#include "hate/visibility.h"

namespace grenade::vx::network::routing::csp {

/**
 * Router Space Factory for a network consiting of crossbars.
 *
 * Simplifies construction of networks consiting of many crossbars nodes.
 */
struct SYMBOL_VISIBLE CrossbarRouterSpaceFactory : public RouterSpaceFactory
{
	/**
	 * Add a source to the routing graph.
	 * @param size Maximum number of different labels supported by the source.
	 */
	grenade::common::VertexOnTopology add_source(size_t number_label_bits, size_t size);

	/**
	 * Add a target to the routing graph.
	 */
	grenade::common::VertexOnTopology add_target();

	/**
	 * Add a crossbar to the routing space.
	 * This adds all the crossbars and crossbar connector vertices to the routing graph that belong
	 * to the crossbar.
	 * @param x_size Number of crossbar columns
	 * @param y_size Number of crossbar rows
	 * @param crossbar_mask_bit_count Number of bits in mask and target of crossbar nodes
	 * @param diagonal_elements Whether to generate diagonal elements in the crossbar
	 */
	CrossbarOnRoutingGraph add_crossbar(
	    size_t x_size,
	    size_t y_size,
	    size_t crossbar_node_bit_count,
	    bool diagonal_elements = true);

	/**
	 * Connect two vertices on the routing graph.
	 */
	void connect(
	    grenade::common::VertexOnTopology const& source,
	    grenade::common::VertexOnTopology const& target);

	/**
	 * Connect a vertex to a crossbar input.
	 * @param source Source vertex
	 * @param crossbar Crossbar
	 * @param channel Input channel on the crossbar
	 */
	void connect_to_crossbar(
	    grenade::common::VertexOnTopology const& source,
	    CrossbarOnRoutingGraph const& crossbar,
	    size_t channel);

	/**
	 * Connect a crossbar output to a vertex.
	 * @param target Target vertex
	 * @param crossbar Crossbar
	 * @param channel Output channel on the crossbar
	 */
	void connect_from_crossbar(
	    grenade::common::VertexOnTopology const& target,
	    CrossbarOnRoutingGraph const& crossbar,
	    size_t channel);

	/**
	 * Connect output of one crossbar to an input of another.
	 * @param source Source crossbar
	 * @param source_channel Output channel on source crossbar.
	 * @param target Target crosbar
	 * @param target_channel Input channel on target crossbar.
	 */
	void connect_crossbars(
	    CrossbarOnRoutingGraph const& source,
	    size_t source_channel,
	    CrossbarOnRoutingGraph const& target,
	    size_t target_channel);

private:
	dapr::AutoKeyMap<CrossbarOnRoutingGraph, Crossbar> m_crossbars;
};

} // namespace grenade::vx::network::routing::csp