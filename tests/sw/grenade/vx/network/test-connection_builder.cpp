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
	ExternalPopulation external_population(2);

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

	// same projection
	descriptor = builder.add(population);

	Projection projection_3(
	    Projection::ReceptorType::excitatory, {{0, 0, Projection::Connection::Weight()}},
	    descriptor, descriptor);

	builder.add(projection_3);
	builder.add(projection_3);

	network = builder.done();
	EXPECT_THROW(grenade::vx::network::build_routing(network), std::runtime_error);

	// overlap
	descriptor = builder.add(population);

	Projection projection_4(
	    Projection::ReceptorType::excitatory,
	    {{0, 0, Projection::Connection::Weight()}, {1, 1, Projection::Connection::Weight()}},
	    descriptor, descriptor);
	Projection projection_5(
	    Projection::ReceptorType::excitatory, {{1, 1, Projection::Connection::Weight()}},
	    descriptor, descriptor);

	builder.add(projection_4);
	builder.add(projection_5);

	network = builder.done();
	EXPECT_THROW(grenade::vx::network::build_routing(network), std::runtime_error);

	// from external no overlap
	auto input_descriptor = builder.add(external_population);
	descriptor = builder.add(population);

	projection_1 = Projection(
	    Projection::ReceptorType::excitatory, {{0, 0, Projection::Connection::Weight()}},
	    input_descriptor, descriptor);
	projection_2 = Projection(
	    Projection::ReceptorType::excitatory, {{1, 1, Projection::Connection::Weight()}},
	    input_descriptor, descriptor);

	builder.add(projection_1);
	builder.add(projection_2);

	network = builder.done();
	EXPECT_NO_THROW(grenade::vx::network::build_routing(network));

	// from external same projection
	input_descriptor = builder.add(external_population);
	descriptor = builder.add(population);

	projection_3 = Projection(
	    Projection::ReceptorType::excitatory, {{0, 0, Projection::Connection::Weight()}},
	    input_descriptor, descriptor);

	builder.add(projection_3);
	builder.add(projection_3);

	network = builder.done();
	EXPECT_THROW(grenade::vx::network::build_routing(network), std::runtime_error);

	// from external overlap
	input_descriptor = builder.add(external_population);
	descriptor = builder.add(population);

	projection_4 = Projection(
	    Projection::ReceptorType::excitatory,
	    {{0, 0, Projection::Connection::Weight()}, {1, 1, Projection::Connection::Weight()}},
	    input_descriptor, descriptor);
	projection_5 = Projection(
	    Projection::ReceptorType::excitatory, {{1, 1, Projection::Connection::Weight()}},
	    input_descriptor, descriptor);

	builder.add(projection_4);
	builder.add(projection_5);

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

TEST(build_routing, EmptyProjection)
{
	NetworkBuilder builder;

	Population population({AtomicNeuronOnDLS(Enum(0))}, {true});

	auto descriptor = builder.add(population);

	// empty projection
	Projection projection(Projection::ReceptorType::excitatory, {}, descriptor, descriptor);

	auto const projection_descriptor = builder.add(projection);

	auto network = builder.done();
	auto const routing_result = grenade::vx::network::build_routing(network);

	EXPECT_TRUE(routing_result.connections.contains(projection_descriptor));
}
