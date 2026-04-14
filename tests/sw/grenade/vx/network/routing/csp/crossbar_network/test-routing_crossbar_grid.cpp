#include <gtest/gtest.h>

#include "grenade/vx/network/routing/csp/factory/crossbar_network.h"
#include "grenade/vx/network/routing/csp/router.h"
#include "grenade/vx/network/routing/csp/vertex/source.h"
#include "hate/timer.h"

#include <iostream>

namespace {
using namespace grenade::vx::network::routing::csp;

auto build_crossbar_grid = [](CrossbarRouterSpaceFactory& factory,
                              size_t num_crossbars_x,
                              size_t num_crossbars_y,
                              size_t size_sources) {
	/**
	 * Crossbar for grid structure.
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
	 */

	// Add sources
	std::vector<std::vector<grenade::common::VertexOnTopology>> sources(num_crossbars_y);
	for (size_t y = 0; y < num_crossbars_y; y++) {
		sources.at(y).reserve(num_crossbars_x);
		for (size_t x = 0; x < num_crossbars_x; x++) {
			sources.at(y).push_back(factory.add_source(16, size_sources));
		}
	}

	// Add targets
	std::vector<std::vector<grenade::common::VertexOnTopology>> targets(num_crossbars_y);
	for (size_t y = 0; y < num_crossbars_y; y++) {
		targets.at(y).reserve(num_crossbars_x);
		for (size_t x = 0; x < num_crossbars_x; x++) {
			targets.at(y).push_back(factory.add_target());
		}
	}

	// Add crossbars
	std::vector<std::vector<CrossbarOnRoutingGraph>> crossbars(num_crossbars_y);
	for (size_t y = 0; y < num_crossbars_y; y++) {
		crossbars.at(y).reserve(num_crossbars_x);
		for (size_t x = 0; x < num_crossbars_x; x++) {
			crossbars.at(y).push_back(factory.add_crossbar(5, 5, false));
			factory.connect_to_crossbar(sources.at(y).at(x), crossbars.at(y).at(x), 0);
			factory.connect_from_crossbar(targets.at(y).at(x), crossbars.at(y).at(x), 0);
		}
	}

	// Horizontal connections
	for (size_t y = 0; y < num_crossbars_y; y++) {
		for (size_t x = 0; x + 1 < num_crossbars_x; x++) {
			factory.connect_crossbars(
			    crossbars.at(y).at(x), 2, crossbars.at(y).at(x + 1), 1); // left to right
			factory.connect_crossbars(
			    crossbars.at(y).at(x + 1), 2, crossbars.at(y).at(x), 1); // right to left
		}
	}

	// Vertical connections
	for (size_t y = 0; y + 1 < num_crossbars_y; y++) {
		for (size_t x = 0; x < num_crossbars_x; x++) {
			factory.connect_crossbars(
			    crossbars.at(y).at(x), 3, crossbars.at(y + 1).at(x), 4); // top to bottom
			factory.connect_crossbars(
			    crossbars.at(y + 1).at(x), 3, crossbars.at(y).at(x), 4); // bottom to top
		}
	}
	return std::pair{sources, targets};
};

} // namespace

TEST(CspRouter, CrossbarGrid)
{
	using namespace grenade::vx::network::routing::csp;

	size_t const number_crossbars_x = 2;
	size_t const number_crossbars_y = 2;
	size_t const size_sources = 4;

	CrossbarRouterSpaceFactory factory;
	auto [sources, targets] =
	    build_crossbar_grid(factory, number_crossbars_x, number_crossbars_y, size_sources);

	std::map<SourceTargetPair, std::map<size_t, size_t>> source_target_pairs;

	source_target_pairs[SourceTargetPair(sources.at(0).at(0), targets.at(0).at(1))].insert({0, 0});

	auto router_space = factory.done();

	Router router;
	hate::Timer timer;
	auto result = router(std::move(router_space));
	EXPECT_LE(timer.get_s(), 30);
}

TEST(CspRouter, CrossbarRow)
{
	using namespace grenade::vx::network::routing::csp;

	size_t const number_crossbars_x = 5;
	size_t const number_crossbars_y = 1;
	size_t const size_sources = 10;

	CrossbarRouterSpaceFactory factory;
	auto [sources, targets] =
	    build_crossbar_grid(factory, number_crossbars_x, number_crossbars_y, size_sources);

	std::map<SourceTargetPair, std::map<size_t, size_t>> source_target_pairs;

	source_target_pairs[SourceTargetPair(sources.at(0).at(0), targets.at(0).at(1))].insert({0, 0});
	source_target_pairs[SourceTargetPair(sources.at(0).at(0), targets.at(0).at(2))].insert({0, 0});
	source_target_pairs[SourceTargetPair(sources.at(0).at(0), targets.at(0).at(3))].insert({0, 0});
	source_target_pairs[SourceTargetPair(sources.at(0).at(0), targets.at(0).at(4))].insert({0, 0});

	source_target_pairs[SourceTargetPair(sources.at(0).at(1), targets.at(0).at(0))].insert({1, 1});
	source_target_pairs[SourceTargetPair(sources.at(0).at(1), targets.at(0).at(2))].insert({1, 1});
	source_target_pairs[SourceTargetPair(sources.at(0).at(1), targets.at(0).at(3))].insert({1, 1});
	source_target_pairs[SourceTargetPair(sources.at(0).at(1), targets.at(0).at(4))].insert({1, 1});


	auto router_space = factory.done();

	Router router;
	hate::Timer timer;
	auto result = router(std::move(router_space));
	EXPECT_LE(timer.get_s(), 30);
}
