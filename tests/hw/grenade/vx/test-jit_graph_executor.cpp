#include <gtest/gtest.h>

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/io_data_list.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/routing_builder.h"
#include "hate/timer.h"
#include "lola/vx/v3/chip.h"
#include <log4cxx/logger.h>

TEST(JITGraphExecutor, Empty)
{
	grenade::vx::Graph g;

	grenade::vx::JITGraphExecutor executor;

	grenade::vx::JITGraphExecutor::ChipConfigs initial_config;

	grenade::vx::IODataMap input_map;

	auto const result_map = grenade::vx::run(executor, g, input_map, initial_config);
	EXPECT_TRUE(result_map.empty());

	grenade::vx::IODataList input_list;
	input_list.from_input_map(input_map, g);

	auto const result_list = grenade::vx::run(executor, g, input_list, initial_config);
	EXPECT_TRUE(result_list.data.empty());
}

TEST(JITGraphExecutor, DifferentialConfig)
{
	// use network to build simple experiment using the hardeware
	grenade::vx::network::NetworkBuilder builder;
	builder.add(
	    grenade::vx::network::Population({halco::hicann_dls::vx::v3::AtomicNeuronOnDLS()}, {true}));
	auto network = builder.done();
	auto routing = grenade::vx::network::build_routing(network);
	auto network_graph = grenade::vx::network::build_network_graph(network, routing);

	// construct JIT executor with differential config mode enabled
	grenade::vx::backend::Connection connection;
	std::map<halco::hicann_dls::vx::v3::DLSGlobal, grenade::vx::backend::Connection> connections;
	connections.emplace(halco::hicann_dls::vx::v3::DLSGlobal(), std::move(connection));
	grenade::vx::JITGraphExecutor executor(std::move(connections), true);

	// a single batch entry with some runtime to ensure use of hardware
	grenade::vx::IODataMap input_map;
	input_map.runtime[grenade::vx::coordinate::ExecutionInstance()].push_back(
	    haldls::vx::Timer::Value(100));

	grenade::vx::JITGraphExecutor::ChipConfigs initial_config{
	    {grenade::vx::coordinate::ExecutionInstance(), lola::vx::v3::Chip()}};
	auto& config = initial_config.at(grenade::vx::coordinate::ExecutionInstance());

	auto logger = log4cxx::Logger::getLogger("TEST_JITGraphExecutor.DifferentialConfig");
	{
		hate::Timer timer;
		grenade::vx::run(executor, network_graph.get_graph(), input_map, initial_config);
		// First run: expect CapMem settling time
		EXPECT_GE(timer.get_ms(), 100);
	}
	// change CapMem cell value
	config.neuron_block.atomic_neurons.front().leak.v_leak =
	    lola::vx::v3::AtomicNeuron::AnalogValue(123);
	{
		hate::Timer timer;
		grenade::vx::run(executor, network_graph.get_graph(), input_map, initial_config);
		// Second run: expect CapMem settling time due to single cell change
		EXPECT_GE(timer.get_ms(), 100);
	}
	// change non-CapMem value
	config.neuron_block.atomic_neurons.front().leak.enable_multiplication =
	    !config.neuron_block.atomic_neurons.front().leak.enable_multiplication;
	{
		hate::Timer timer;
		grenade::vx::run(executor, network_graph.get_graph(), input_map, initial_config);
		// Third run: expect no CapMem settling time due to non-CapMem change
		EXPECT_LE(timer.get_ms(), 100);
		// Not too fast (may change)
		EXPECT_GE(timer.get_ms(), 5);
	}
	{
		hate::Timer timer;
		grenade::vx::run(executor, network_graph.get_graph(), input_map, initial_config);
		// Fourth run: expect no CapMem settling time due to non-CapMem change and even faster
		// construction due to equality of config
		EXPECT_LE(timer.get_ms(), 5);
	}
}
