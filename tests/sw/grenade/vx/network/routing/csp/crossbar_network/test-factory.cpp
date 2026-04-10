#include <gtest/gtest.h>

#include "grenade/vx/network/routing/csp/factory/crossbar_network.h"

#include <iostream>

TEST(CspRouter, CrossbarFactory)
{
	using namespace grenade::vx::network::routing::csp;

	CrossbarRouterSpaceFactory factory;

	/**
	 * Test with simplified crossbars of shape
	 * source ->  x   x   x
	 * left   ->  x   x   x
	 * right  ->  x   x   x
	 *            |   |   |
	 *            v   v   v
	 *            t   l   r
	 *            a   e   i
	 *            r   f   g
	 *            g   t   h
	 *            e       t
	 *            t
	 *
	 * The full network is a chain of four crossbars each with a source and target.
	 */


	size_t const num_nodes = 4;
	size_t const size_sources = 10;

	size_t const crossbar_x_size = 3;
	size_t const crossbar_y_size = 3;

	std::vector<grenade::common::VertexOnTopology> sources;
	for (size_t i = 0; i < num_nodes; i++) {
		sources.push_back(factory.add_source(16, size_sources));
	}

	std::vector<grenade::common::VertexOnTopology> targets;
	for (size_t i = 0; i < num_nodes; i++) {
		targets.push_back(factory.add_target());
	}

	std::vector<CrossbarOnRoutingGraph> crossbars;
	for (size_t i = 0; i < num_nodes; i++) {
		crossbars.push_back(factory.add_crossbar(crossbar_x_size, crossbar_y_size, 16, false));
		factory.connect_to_crossbar(sources.at(i), crossbars.at(i), 0);
		factory.connect_from_crossbar(targets.at(i), crossbars.at(i), 0);
	}

	for (size_t i = 0; i < num_nodes - 1; i++) {
		factory.connect_crossbars(crossbars.at(i), 2, crossbars.at(i + 1), 1);
		factory.connect_crossbars(crossbars.at(i + 1), 1, crossbars.at(i), 2);
	}

	auto router_space = factory.done();

	EXPECT_EQ(
	    router_space->get_graph().num_vertices(),
	    (num_nodes * (crossbar_x_size - 1) * crossbar_y_size) + /*Crossbar nodes*/
	        (num_nodes * (crossbar_x_size + crossbar_y_size)) + /*Crossbar IO*/
	        (2 * num_nodes));                                   /*Sources and targets*/

	EXPECT_EQ(
	    router_space->get_graph().num_edges(),
	    (2 * num_nodes * (crossbar_x_size - 1) *
	     crossbar_y_size) +         /*Crossbar IO and crossbar nodes*/
	        (2 * num_nodes) +       /*Sources and targets */
	        (2 * (num_nodes - 1))); /*Crossbars*/

	auto [routes, routes_cyclic] = RouteCollector()(router_space->get_graph());

	// Every source is connected to every target but its own
	EXPECT_EQ(routes.size(), num_nodes * (num_nodes - 1));

	// No cyclic routes in network
	EXPECT_EQ(routes_cyclic.size(), 0);
}
