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
