#include "grenade/vx/network/routing/csp/propagator/bitwise_and.h"
#include "grenade/vx/network/routing/csp/router.h"
#include "grenade/vx/network/routing/csp/router_space_factory.h"
#include "grenade/vx/network/routing/csp/source_target_pair.h"
#include "grenade/vx/network/routing/csp/vertex/filter.h"
#include "grenade/vx/network/routing/csp/vertex/source.h"
#include "grenade/vx/network/routing/csp/vertex/target.h"
#include "hate/timer.h"

#include <gtest/gtest.h>

namespace {
using namespace grenade::vx::network::routing::csp;

struct DummyFilter : public Filter
{
	DummyFilter(Gecode::Space& space) :
	    mask(Gecode::IntVar(space, 0, (1 << 16) - 1)),
	    target(Gecode::IntVar(space, 0, (1 << 16) - 1))
	{
	}

	~DummyFilter() {}

	Gecode::BoolVar apply_filter(Gecode::Home space, Gecode::IntVar value)
	{
		Gecode::IntVar tmp(space, 0, (1 << 16) - 1);
		propagator::bitwise_and(space, value, mask, tmp);
		Gecode::BoolVar ret(space, 0, 1);
		Gecode::rel(space, tmp, Gecode::IRT_EQ, target, ret);
		return ret;
	}

	void update(Gecode::Space& space, RoutingVertex& other)
	{
		auto& other_casted = dynamic_cast<DummyFilter&>(other);
		mask.update(space, other_casted.mask);
		target.update(space, other_casted.target);
	}

	Gecode::IntVarArgs get_parameters() const
	{
		Gecode::IntVarArgs ret;
		ret << mask;
		ret << target;
		return ret;
	}

	std::string get_name() const
	{
		return "DummyFilter";
	}

	std::vector<std::string> get_parameter_names() const
	{
		return std::vector<std::string>{"Mask", "Target"};
	}

	std::unique_ptr<RoutingVertex> copy() const
	{
		return std::make_unique<DummyFilter>(*this);
	}

	std::unique_ptr<RoutingVertex> move()
	{
		return std::make_unique<DummyFilter>(std::move(*this));
	}

	bool is_equal_to(RoutingVertex const&) const
	{
		return false;
	}

	std::vector<size_t> get_parameter_bitcounts() const
	{
		return std::vector<size_t>{2, 16};
	}

	Gecode::IntVar mask;
	Gecode::IntVar target;
};

struct DummyRouterSpaceFactory : public RouterSpaceFactory
{
	grenade::common::VertexOnTopology add_source(size_t size)
	{
		return get_graph().add_vertex(Source(16, size, get_space()));
	}

	grenade::common::VertexOnTopology add_target()
	{
		return get_graph().add_vertex(Target());
	}

	grenade::common::VertexOnTopology add_filter()
	{
		return get_graph().add_vertex(DummyFilter(get_space()));
	}

	grenade::common::EdgeOnTopology connect(
	    grenade::common::VertexOnTopology const& source,
	    grenade::common::VertexOnTopology const& target)
	{
		return get_graph().add_edge(source, target, grenade::common::Edge());
	}
};

auto build_crossbar_filter_network = [](DummyRouterSpaceFactory& factory) {
	/**
	 * Network with crossbar structure
	 * s0 -> f00 - f01
	 *        |     |
	 * s1 -> f10 - f11
	 *        |     |
	 *        v     v
	 *        t0    t1
	 */

	std::vector<grenade::common::VertexOnTopology> sources;
	std::vector<grenade::common::VertexOnTopology> targets;

	sources.push_back(factory.add_source(3));
	sources.push_back(factory.add_source(3));

	auto filter_0_0 = factory.add_filter();
	auto filter_0_1 = factory.add_filter();
	auto filter_1_0 = factory.add_filter();
	auto filter_1_1 = factory.add_filter();

	targets.push_back(factory.add_target());
	targets.push_back(factory.add_target());

	factory.connect(sources.at(0), filter_0_0);
	factory.connect(sources.at(0), filter_0_1);
	factory.connect(sources.at(1), filter_1_0);
	factory.connect(sources.at(1), filter_1_1);

	factory.connect(filter_0_0, targets.at(0));
	factory.connect(filter_0_1, targets.at(1));
	factory.connect(filter_1_0, targets.at(0));
	factory.connect(filter_1_1, targets.at(1));
	return std::pair{sources, targets};
};

} // namespace

TEST(CspRouter, NoConnections)
{
	using namespace grenade::vx::network::routing::csp;

	DummyRouterSpaceFactory factory;
	auto [sources, targets] = build_crossbar_filter_network(factory);

	std::map<SourceTargetPair, std::map<size_t, size_t>> source_target_pairs;

	factory.build_constraints(source_target_pairs);
	auto router_space = factory.done();

	Router router;
	hate::Timer timer;
	EXPECT_NO_THROW(router(std::move(router_space)));
	EXPECT_LE(timer.get_s(), 30);
}

TEST(CspRouter, RouteCollection)
{
	using namespace grenade::vx::network::routing::csp;

	DummyRouterSpaceFactory factory;
	auto [sources, targets] = build_crossbar_filter_network(factory);

	std::map<SourceTargetPair, std::map<size_t, size_t>> source_target_pairs;
	source_target_pairs[SourceTargetPair(sources.at(0), targets.at(0))].insert({0, 0});
	source_target_pairs[SourceTargetPair(sources.at(0), targets.at(0))].insert({1, 1});
	source_target_pairs[SourceTargetPair(sources.at(1), targets.at(1))].insert({0, 0});

	factory.build_constraints(source_target_pairs);
	auto router_space = factory.done();
	auto [routes, routes_cyclic] = RouteCollector()(router_space->get_graph());

	EXPECT_EQ(routes.size(), 2 /*Sources*/ * 2 /*Targets*/);
	EXPECT_EQ(routes_cyclic.size(), 0);
	for (auto const& [_, route] : routes) {
		EXPECT_EQ(route.size(), 1);
	}

	Router router;
	hate::Timer timer;
	EXPECT_NO_THROW(router(std::move(router_space)));
	EXPECT_LE(timer.get_s(), 60);
}

TEST(CspRouter, MergingOnTarget)
{
	using namespace grenade::vx::network::routing::csp;

	DummyRouterSpaceFactory factory;
	auto [sources, targets] = build_crossbar_filter_network(factory);

	std::map<SourceTargetPair, std::map<size_t, size_t>> source_target_pairs;
	// From source 0 and source 1 to same target label
	source_target_pairs[SourceTargetPair(sources.at(0), targets.at(0))].insert({0, 0});
	source_target_pairs[SourceTargetPair(sources.at(1), targets.at(0))].insert({1, 0});

	// From source 0 and source 1 to same target label
	source_target_pairs[SourceTargetPair(sources.at(0), targets.at(1))].insert({1, 2});
	source_target_pairs[SourceTargetPair(sources.at(1), targets.at(1))].insert({0, 2});

	// Single source index to single target index
	source_target_pairs[SourceTargetPair(sources.at(0), targets.at(0))].insert({2, 1});

	factory.build_constraints(source_target_pairs);
	auto router_space = factory.done();

	Router router;
	hate::Timer timer;
	EXPECT_NO_THROW(router(std::move(router_space)));
	EXPECT_LE(timer.get_s(), 60);
}