#include <gtest/gtest.h>

#include "grenade/vx/logical_network/network_builder.h"
#include "grenade/vx/logical_network/network_graph_builder.h"

using namespace grenade::vx::logical_network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(logical_network_build_network_graph, Multapses)
{
	NetworkBuilder builder;

	Population population({AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1))}, {true, true});

	auto descriptor = builder.add(population);

	constexpr size_t max_weight_multiplier = 4;
	Projection::Connections connections{
	    Projection::Connection(0, 0, Projection::Connection::Weight(12)),
	    Projection::Connection(0, 1, Projection::Connection::Weight(100)),
	    Projection::Connection(1, 0, Projection::Connection::Weight(54)),
	    Projection::Connection(
	        1, 1,
	        Projection::Connection::Weight(
	            grenade::vx::network::Projection::Connection::Weight::max * max_weight_multiplier)),
	};
	Projection projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), connections, descriptor, descriptor);

	auto const projection_descriptor = builder.add(projection);

	auto network = builder.done();

	auto const network_graph = build_network_graph(network);

	EXPECT_TRUE(network_graph.get_hardware_network());
	EXPECT_EQ(network_graph.get_hardware_network()->projections.size(), max_weight_multiplier);
	EXPECT_EQ(
	    network_graph.get_hardware_network()
	        ->projections.at(grenade::vx::network::ProjectionDescriptor(0))
	        .connections.size(),
	    max_weight_multiplier);
	EXPECT_EQ(
	    network_graph.get_hardware_network()
	        ->projections.at(grenade::vx::network::ProjectionDescriptor(1))
	        .connections.size(),
	    2);
	EXPECT_EQ(
	    network_graph.get_hardware_network()
	        ->projections.at(grenade::vx::network::ProjectionDescriptor(2))
	        .connections.size(),
	    1);
	EXPECT_EQ(
	    network_graph.get_hardware_network()
	        ->projections.at(grenade::vx::network::ProjectionDescriptor(3))
	        .connections.size(),
	    1);
	{
		auto const translation = network_graph.get_projection_translation().equal_range(
		    std::make_pair(projection_descriptor, 0));
		EXPECT_EQ(std::distance(translation.first, translation.second), 1);
		EXPECT_EQ(
		    connections.at(0).weight.value(), network_graph.get_hardware_network()
		                                          ->projections.at(translation.first->second.first)
		                                          .connections.at(translation.first->second.second)
		                                          .weight.value());
	}
	{
		auto translation = network_graph.get_projection_translation().equal_range(
		    std::make_pair(projection_descriptor, 1));
		EXPECT_EQ(std::distance(translation.first, translation.second), 2);
		EXPECT_EQ(
		    grenade::vx::network::Projection::Connection::Weight::max,
		    network_graph.get_hardware_network()
		        ->projections.at(translation.first->second.first)
		        .connections.at(translation.first->second.second)
		        .weight.value());
		++translation.first;
		EXPECT_EQ(
		    connections.at(1).weight.value() -
		        grenade::vx::network::Projection::Connection::Weight::max,
		    network_graph.get_hardware_network()
		        ->projections.at(translation.first->second.first)
		        .connections.at(translation.first->second.second)
		        .weight.value());
	}
	{
		auto const translation = network_graph.get_projection_translation().equal_range(
		    std::make_pair(projection_descriptor, 2));
		EXPECT_EQ(std::distance(translation.first, translation.second), 1);
		EXPECT_EQ(
		    connections.at(2).weight.value(), network_graph.get_hardware_network()
		                                          ->projections.at(translation.first->second.first)
		                                          .connections.at(translation.first->second.second)
		                                          .weight.value());
	}
	{
		auto translation = network_graph.get_projection_translation().equal_range(
		    std::make_pair(projection_descriptor, 3));
		EXPECT_EQ(std::distance(translation.first, translation.second), 4);
		EXPECT_EQ(
		    grenade::vx::network::Projection::Connection::Weight::max,
		    network_graph.get_hardware_network()
		        ->projections.at(translation.first->second.first)
		        .connections.at(translation.first->second.second)
		        .weight.value());
		++translation.first;
		EXPECT_EQ(
		    grenade::vx::network::Projection::Connection::Weight::max,
		    network_graph.get_hardware_network()
		        ->projections.at(translation.first->second.first)
		        .connections.at(translation.first->second.second)
		        .weight.value());
		++translation.first;
		EXPECT_EQ(
		    grenade::vx::network::Projection::Connection::Weight::max,
		    network_graph.get_hardware_network()
		        ->projections.at(translation.first->second.first)
		        .connections.at(translation.first->second.second)
		        .weight.value());
		++translation.first;
		EXPECT_EQ(
		    grenade::vx::network::Projection::Connection::Weight::max,
		    network_graph.get_hardware_network()
		        ->projections.at(translation.first->second.first)
		        .connections.at(translation.first->second.second)
		        .weight.value());
	}
}
