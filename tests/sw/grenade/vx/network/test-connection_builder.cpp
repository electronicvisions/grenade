#include <gtest/gtest.h>

#include "grenade/vx/network/connection_builder.h"
#include "grenade/vx/network/network_builder.h"

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v2;
using namespace halco::common;

TEST(build_routing, ProjectionOverlap)
{
	NetworkBuilder builder;

	Population population({AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1))}, {true, true});

	// no overlap
	auto descriptor = builder.add(population);

	Projection projection_1(
	    Projection::ReceptorType::excitatory, {{0, 0, Projection::Connection::Weight()}},
	    descriptor, descriptor);
	Projection projection_2(
	    Projection::ReceptorType::excitatory, {{1, 1, Projection::Connection::Weight()}},
	    descriptor, descriptor);

	builder.add(projection_1);
	builder.add(projection_2);

	auto network = builder.done();
	EXPECT_NO_THROW(grenade::vx::network::build_routing(network));

	// overlap
	descriptor = builder.add(population);

	Projection projection_3(
	    Projection::ReceptorType::excitatory, {{0, 0, Projection::Connection::Weight()}},
	    descriptor, descriptor);

	builder.add(projection_3);
	builder.add(projection_3);

	network = builder.done();
	EXPECT_THROW(grenade::vx::network::build_routing(network), std::runtime_error);
}

TEST(build_routing, WeightOutOfRange)
{
	NetworkBuilder builder;

	Population population({AtomicNeuronOnDLS(Enum(0))}, {true});

	auto descriptor = builder.add(population);

	Projection projection(
	    Projection::ReceptorType::excitatory, {{0, 0, Projection::Connection::Weight(1234)}},
	    descriptor, descriptor);

	builder.add(projection);

	auto network = builder.done();
	EXPECT_THROW(grenade::vx::network::build_routing(network), std::out_of_range);
}
