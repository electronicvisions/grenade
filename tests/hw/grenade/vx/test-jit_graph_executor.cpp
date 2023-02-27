#include <gtest/gtest.h>

#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/io_data_list.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/routing_builder.h"
#include "grenade/vx/signal_flow/graph.h"
#include "hate/timer.h"
#include "lola/vx/v3/chip.h"
#include <future>
#include <log4cxx/logger.h>

TEST(JITGraphExecutor, Empty)
{
	grenade::vx::signal_flow::Graph g;

	grenade::vx::execution::JITGraphExecutor executor;

	grenade::vx::execution::JITGraphExecutor::ChipConfigs initial_config;

	grenade::vx::IODataMap input_map;

	auto const result_map = grenade::vx::execution::run(executor, g, input_map, initial_config);
	EXPECT_TRUE(result_map.empty());

	grenade::vx::IODataList input_list;
	input_list.from_input_map(input_map, g);

	auto const result_list = grenade::vx::execution::run(executor, g, input_list, initial_config);
	EXPECT_TRUE(result_list.data.empty());
}

constexpr static long capmem_settling_time_ms = 100;

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
	grenade::vx::execution::backend::Connection connection;
	std::map<halco::hicann_dls::vx::v3::DLSGlobal, grenade::vx::execution::backend::Connection>
	    connections;
	connections.emplace(halco::hicann_dls::vx::v3::DLSGlobal(), std::move(connection));
	grenade::vx::execution::JITGraphExecutor executor(std::move(connections), true);

	// a single batch entry with some runtime to ensure use of hardware
	grenade::vx::IODataMap input_map;
	input_map.runtime[grenade::vx::signal_flow::ExecutionInstance()].push_back(
	    haldls::vx::Timer::Value(100));

	grenade::vx::execution::JITGraphExecutor::ChipConfigs initial_config{
	    {grenade::vx::signal_flow::ExecutionInstance(), lola::vx::v3::Chip()}};
	auto& config = initial_config.at(grenade::vx::signal_flow::ExecutionInstance());

	auto logger = log4cxx::Logger::getLogger("TEST_JITGraphExecutor.DifferentialConfig");
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, network_graph.get_graph(), input_map, initial_config);
		// First run: expect CapMem settling time
		EXPECT_GE(timer.get_ms(), capmem_settling_time_ms);
	}
	// change CapMem cell value
	config.neuron_block.atomic_neurons.front().leak.v_leak =
	    lola::vx::v3::AtomicNeuron::AnalogValue(123);
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, network_graph.get_graph(), input_map, initial_config);
		// Second run: expect CapMem settling time due to single cell change
		EXPECT_GE(timer.get_ms(), capmem_settling_time_ms);
	}
	// change non-CapMem value
	config.neuron_block.atomic_neurons.front().leak.enable_multiplication =
	    !config.neuron_block.atomic_neurons.front().leak.enable_multiplication;
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, network_graph.get_graph(), input_map, initial_config);
		// Third run: expect no CapMem settling time due to non-CapMem change
		EXPECT_LE(timer.get_ms(), capmem_settling_time_ms);
		// Not too fast (may change)
		EXPECT_GE(timer.get_ms(), 5);
	}
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, network_graph.get_graph(), input_map, initial_config);
		// Fourth run: expect no CapMem settling time due to non-CapMem change and even faster
		// construction due to equality of config
		EXPECT_LE(timer.get_ms(), 5);
	}
}

TEST(JITGraphExecutor, NoDifferentialConfig)
{
	// use network to build simple experiment using the hardeware
	grenade::vx::network::NetworkBuilder builder;
	builder.add(
	    grenade::vx::network::Population({halco::hicann_dls::vx::v3::AtomicNeuronOnDLS()}, {true}));
	auto network = builder.done();
	auto routing = grenade::vx::network::build_routing(network);
	auto network_graph = grenade::vx::network::build_network_graph(network, routing);

	// construct JIT executor with differential config mode disabled
	grenade::vx::execution::backend::Connection connection;
	std::map<halco::hicann_dls::vx::v3::DLSGlobal, grenade::vx::execution::backend::Connection>
	    connections;
	connections.emplace(halco::hicann_dls::vx::v3::DLSGlobal(), std::move(connection));
	grenade::vx::execution::JITGraphExecutor executor(std::move(connections), false);

	// a single batch entry with some runtime to ensure use of hardware
	grenade::vx::IODataMap input_map;
	input_map.runtime[grenade::vx::signal_flow::ExecutionInstance()].push_back(
	    haldls::vx::Timer::Value(100));

	grenade::vx::execution::JITGraphExecutor::ChipConfigs initial_config{
	    {grenade::vx::signal_flow::ExecutionInstance(), lola::vx::v3::Chip()}};
	auto& config = initial_config.at(grenade::vx::signal_flow::ExecutionInstance());

	auto logger = log4cxx::Logger::getLogger("TEST_JITGraphExecutor.NoDifferentialConfig");
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, network_graph.get_graph(), input_map, initial_config);
		// First run: expect CapMem settling time
		EXPECT_GE(timer.get_ms(), capmem_settling_time_ms);
	}
	// change CapMem cell value
	config.neuron_block.atomic_neurons.front().leak.v_leak =
	    lola::vx::v3::AtomicNeuron::AnalogValue(123);
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, network_graph.get_graph(), input_map, initial_config);
		// Second run: expect CapMem settling time
		EXPECT_GE(timer.get_ms(), capmem_settling_time_ms);
	}
	// change non-CapMem value
	config.neuron_block.atomic_neurons.front().leak.enable_multiplication =
	    !config.neuron_block.atomic_neurons.front().leak.enable_multiplication;
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, network_graph.get_graph(), input_map, initial_config);
		// Third run: expect CapMem settling time
		EXPECT_GE(timer.get_ms(), capmem_settling_time_ms);
	}
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, network_graph.get_graph(), input_map, initial_config);
		// Fourth run: expect CapMem settling time
		EXPECT_GE(timer.get_ms(), capmem_settling_time_ms);
	}
}

TEST(JITGraphExecutor, ConcurrentUsage)
{
	// use network to build simple experiment using the hardeware
	grenade::vx::network::NetworkBuilder builder;
	builder.add(
	    grenade::vx::network::Population({halco::hicann_dls::vx::v3::AtomicNeuronOnDLS()}, {true}));
	auto const network = builder.done();
	auto const routing = grenade::vx::network::build_routing(network);
	auto const network_graph = grenade::vx::network::build_network_graph(network, routing);

	// construct JIT executor with differential config mode enabled
	grenade::vx::execution::backend::Connection connection;
	std::map<halco::hicann_dls::vx::v3::DLSGlobal, grenade::vx::execution::backend::Connection>
	    connections;
	connections.emplace(halco::hicann_dls::vx::v3::DLSGlobal(), std::move(connection));
	grenade::vx::execution::JITGraphExecutor executor(std::move(connections), true);

	// a single batch entry with some runtime to ensure use of hardware
	grenade::vx::IODataMap input_map;
	input_map.runtime[grenade::vx::signal_flow::ExecutionInstance()].push_back(
	    haldls::vx::Timer::Value(10000 * haldls::vx::Timer::Value::fpga_clock_cycles_per_us));

	grenade::vx::execution::JITGraphExecutor::ChipConfigs initial_config{
	    {grenade::vx::signal_flow::ExecutionInstance(), lola::vx::v3::Chip()}};

	std::vector<std::future<grenade::vx::IODataMap>> results;
	constexpr size_t num_concurrent = 100;

	auto const run_func = [&]() -> grenade::vx::IODataMap {
		return grenade::vx::execution::run(
		    executor, network_graph.get_graph(), input_map, initial_config);
	};

	hate::Timer timer;
	for (size_t i = 0; i < num_concurrent; ++i) {
		results.push_back(std::async(std::launch::async, run_func));
	}

	for (size_t i = 0; i < num_concurrent; ++i) {
		auto const data = results.at(i).get();
		assert(data.execution_time_info);
		EXPECT_TRUE(data.execution_time_info->execution_duration_per_hardware.contains(
		    halco::hicann_dls::vx::v3::DLSGlobal()));
		// expect at least the realtime runtime as duration
		EXPECT_GE(
		    data.execution_time_info->execution_duration_per_hardware.at(
		        halco::hicann_dls::vx::v3::DLSGlobal()),
		    std::chrono::milliseconds(10));
		// expect no more than 200ms per execution
		// since we don't know which execution will be the first, we can't assert, that this one
		// requires more time due to applying the initial config
		EXPECT_LE(
		    data.execution_time_info->execution_duration_per_hardware.at(
		        halco::hicann_dls::vx::v3::DLSGlobal()),
		    std::chrono::milliseconds(200));
	}

	// expect no more than three times the realtime runtime as duration
	// this ensures, that differential mode works concurrently, since otherwise we would expect at
	// least 100ms per execution
	EXPECT_LE(timer.get_ms(), 30 * num_concurrent);
}
