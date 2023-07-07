#include <gtest/gtest.h>

#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/routing/portfolio_router.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/io_data_list.h"
#include "grenade/vx/signal_flow/io_data_map.h"
#include "hate/timer.h"
#include "lola/vx/v3/chip.h"
#include <future>
#include <log4cxx/logger.h>


using namespace halco::hicann_dls::vx::v3;
using namespace grenade::vx::network;

TEST(JITGraphExecutor, Empty)
{
	grenade::vx::signal_flow::Graph g;

	grenade::vx::execution::JITGraphExecutor executor;

	grenade::vx::execution::JITGraphExecutor::ChipConfigs initial_config;

	grenade::vx::signal_flow::IODataMap input_map;

	auto const result_map = grenade::vx::execution::run(executor, g, input_map, initial_config);
	EXPECT_TRUE(result_map.empty());

	grenade::vx::signal_flow::IODataList input_list;
	input_list.from_input_map(input_map, g);

	auto const result_list = grenade::vx::execution::run(executor, g, input_list, initial_config);
	EXPECT_TRUE(result_list.data.empty());
}

constexpr static long capmem_settling_time_ms = 100;

TEST(JITGraphExecutor, DifferentialConfig)
{
	// use network to build simple experiment using the hardware
	NetworkBuilder builder;
	Population::Neurons neurons{Population::Neuron(
	    LogicalNeuronOnDLS(
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	        AtomicNeuronOnDLS()),
	    Population::Neuron::Compartments{
	        {CompartmentOnLogicalNeuron(),
	         Population::Neuron::Compartment{
	             Population::Neuron::Compartment::SpikeMaster(0, true),
	             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})};
	Population population_internal{std::move(neurons)};
	builder.add(population_internal);
	auto network = builder.done();
	auto routing = routing::PortfolioRouter()(network);
	auto network_graph = build_network_graph(network, routing);

	// construct JIT executor with differential config mode enabled
	grenade::vx::execution::JITGraphExecutor executor(true);

	// a single batch entry with some runtime to ensure use of hardware
	grenade::vx::signal_flow::IODataMap input_map;
	input_map.runtime.push_back(
	    {{grenade::vx::signal_flow::ExecutionInstance(), grenade::vx::common::Time(100)}});

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
	NetworkBuilder builder;
	Population::Neurons neurons{Population::Neuron(
	    LogicalNeuronOnDLS(
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	        AtomicNeuronOnDLS()),
	    Population::Neuron::Compartments{
	        {CompartmentOnLogicalNeuron(),
	         Population::Neuron::Compartment{
	             Population::Neuron::Compartment::SpikeMaster(0, true),
	             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})};
	Population population_internal{std::move(neurons)};
	builder.add(population_internal);
	auto network = builder.done();
	auto routing = routing::PortfolioRouter()(network);
	auto network_graph = build_network_graph(network, routing);

	// construct JIT executor with differential config mode disabled
	grenade::vx::execution::JITGraphExecutor executor(false);

	// a single batch entry with some runtime to ensure use of hardware
	grenade::vx::signal_flow::IODataMap input_map;
	input_map.runtime.push_back(
	    {{grenade::vx::signal_flow::ExecutionInstance(), grenade::vx::common::Time(100)}});

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
	NetworkBuilder builder;
	Population::Neurons neurons{Population::Neuron(
	    LogicalNeuronOnDLS(
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	        AtomicNeuronOnDLS()),
	    Population::Neuron::Compartments{
	        {CompartmentOnLogicalNeuron(),
	         Population::Neuron::Compartment{
	             Population::Neuron::Compartment::SpikeMaster(0, true),
	             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})};
	Population population_internal{std::move(neurons)};
	builder.add(population_internal);
	auto network = builder.done();
	auto routing = routing::PortfolioRouter()(network);
	auto network_graph = build_network_graph(network, routing);

	// construct JIT executor with differential config mode enabled
	grenade::vx::execution::JITGraphExecutor executor(true);

	// a single batch entry with some runtime to ensure use of hardware
	grenade::vx::signal_flow::IODataMap input_map;
	input_map.runtime.push_back(
	    {{grenade::vx::signal_flow::ExecutionInstance(),
	      grenade::vx::common::Time(10000 * grenade::vx::common::Time::fpga_clock_cycles_per_us)}});

	grenade::vx::execution::JITGraphExecutor::ChipConfigs initial_config{
	    {grenade::vx::signal_flow::ExecutionInstance(), lola::vx::v3::Chip()}};

	std::vector<std::future<grenade::vx::signal_flow::IODataMap>> results;
	constexpr size_t num_concurrent = 100;

	auto const run_func = [&]() -> grenade::vx::signal_flow::IODataMap {
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
	}

	// expect no more than three times the realtime runtime as duration
	// this ensures, that differential mode works concurrently, since otherwise we would expect at
	// least 100ms per execution
	EXPECT_LE(timer.get_ms(), 3 * 10 * num_concurrent);
}
