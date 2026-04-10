#include <gtest/gtest.h>

#include "grenade/vx/network/routing/csp/factory/crossbar_network.h"
#include "grenade/vx/network/routing/csp/router.h"
#include "hate/timer.h"

#include <iostream>

namespace {
using namespace grenade::vx::network::routing::csp;

auto build_crossbar_chain =
    [](CrossbarRouterSpaceFactory& factory, size_t num_nodes, size_t size_sources) {
	    /**
	     * Crossbar for chain structure.
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
		    crossbars.push_back(factory.add_crossbar(3, 3, 16, false));
		    factory.connect_to_crossbar(sources.at(i), crossbars.at(i), 0);
		    factory.connect_from_crossbar(targets.at(i), crossbars.at(i), 0);
	    }

	    for (size_t i = 0; i < num_nodes - 1; i++) {
		    factory.connect_crossbars(crossbars.at(i), 2, crossbars.at(i + 1), 1);
		    factory.connect_crossbars(crossbars.at(i + 1), 1, crossbars.at(i), 2);
	    }
	    return std::pair{sources, targets};
    };

} // namespace

TEST(CspRouter, CrossbarChainAllToAll)
{
	using namespace grenade::vx::network::routing::csp;


	size_t const num_nodes = 4;
	size_t const size_sources = 10;

	CrossbarRouterSpaceFactory factory;
	auto [sources, targets] = build_crossbar_chain(factory, num_nodes, size_sources);

	std::map<SourceTargetPair, std::map<size_t, size_t>> source_target_pairs;
	// Each source has one connection to each target but its own
	for (size_t source = 0; source < sources.size(); source++) {
		for (size_t target = 0; target < targets.size(); target++) {
			if (source == target) {
				continue;
			}
			source_target_pairs[SourceTargetPair(sources.at(source), targets.at(target))].insert(
			    {0, 0});
		}
	}

	auto router_space = factory.done();

	Router router;
	hate::Timer timer;
	EXPECT_NO_THROW(router(std::move(router_space)));
	EXPECT_LE(timer.get_s(), 30);
}