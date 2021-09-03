#include <gtest/gtest.h>

#include "grenade/vx/network/connection_builder.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph_builder.h"

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v2;
using namespace halco::common;

TEST(build_network_graph, EmptyProjection)
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

	[[maybe_unused]] auto const network_graph = build_network_graph(network, routing_result);
}

TEST(update_network_graph, ProjectionOnBothHemispheres)
{
	NetworkBuilder builder;

	Population population(
	    {AtomicNeuronOnDLS(NeuronColumnOnDLS(0), NeuronRowOnDLS::top),
	     AtomicNeuronOnDLS(NeuronColumnOnDLS(0), NeuronRowOnDLS::bottom)},
	    {true, true});

	auto const descriptor = builder.add(population);

	Projection projection(
	    Projection::ReceptorType::excitatory,
	    {{0, 0, grenade::vx::network::Projection::Connection::Weight(63)},
	     {0, 1, grenade::vx::network::Projection::Connection::Weight(63)},
	     {1, 0, grenade::vx::network::Projection::Connection::Weight(63)},
	     {1, 1, grenade::vx::network::Projection::Connection::Weight(63)}},
	    descriptor, descriptor);

	builder.add(projection);
	auto const network = builder.done();

	auto const routing_result = grenade::vx::network::build_routing(network);

	auto network_graph = build_network_graph(network, routing_result);

	// alter weights
	for (auto& connection : projection.connections) {
		connection.weight = grenade::vx::network::Projection::Connection::Weight(0);
	}
	builder.add(population);
	builder.add(projection);
	auto const new_network = builder.done();

	EXPECT_NO_THROW(grenade::vx::network::update_network_graph(network_graph, new_network));
}
