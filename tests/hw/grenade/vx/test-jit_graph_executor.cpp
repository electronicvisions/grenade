#include <gtest/gtest.h>

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/input_data.h"
#include "grenade/common/output_data.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/topology.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/execution_instance_global.h"
#include "grenade/vx/signal_flow/vertex/chip.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "hate/timer.h"
#include "lola/vx/v3/chip.h"
#include <future>
#include <log4cxx/logger.h>


using namespace halco::hicann_dls::vx::v3;
using namespace grenade::vx::network;

TEST(JITGraphExecutor, Empty)
{
	auto topology = std::make_shared<grenade::common::Topology>();

	grenade::vx::execution::JITGraphExecutor executor;

	grenade::common::InputData input_data;


	auto const results = grenade::vx::execution::run(executor, topology, input_data);
	EXPECT_TRUE(results.ports.empty());
}

constexpr static long capmem_settling_time_ms = 100;

// disabled due to high performance variance on cluster
TEST(JITGraphExecutor, DISABLED_DifferentialConfig)
{
	auto topology = std::make_shared<grenade::common::Topology>();
	auto const chip_descriptor = topology->add_vertex(grenade::vx::signal_flow::vertex::Chip{
	    grenade::vx::common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(),
	    grenade::common::ExecutionInstanceOnExecutor(
	        grenade::common::ExecutionInstanceID(), grenade::common::ConnectionOnExecutor())});
	grenade::common::InputData input_data;
	lola::vx::v3::Chip config;
	input_data.ports.set(
	    {chip_descriptor, 0}, grenade::vx::signal_flow::vertex::Chip::Parameterization(config));

	// construct JIT executor with differential config mode enabled
	grenade::vx::execution::JITGraphExecutor executor(true);

	// a single batch entry with some runtime to ensure use of hardware
	input_data.time_domain_runtimes.set(
	    grenade::common::TimeDomainOnTopology(),
	    grenade::vx::network::abstract::ClockCycleTimeDomainRuntimes(
	        {grenade::vx::common::Time(100)}, grenade::vx::common::Time()));

	auto logger = log4cxx::Logger::getLogger("TEST_JITGraphExecutor.DifferentialConfig");
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, topology, input_data);
		// First run: expect CapMem settling time
		EXPECT_GE(timer.get_ms(), capmem_settling_time_ms);
	}
	// change CapMem cell value
	config.neuron_block.atomic_neurons.front().leak.v_leak =
	    lola::vx::v3::AtomicNeuron::AnalogValue(123);
	input_data.ports.set(
	    {chip_descriptor, 0}, grenade::vx::signal_flow::vertex::Chip::Parameterization(config));
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, topology, input_data);
		// Second run: expect CapMem settling time due to single cell change
		EXPECT_GE(timer.get_ms(), capmem_settling_time_ms);
	}
	// change non-CapMem value
	config.neuron_block.atomic_neurons.front().leak.enable_multiplication =
	    !config.neuron_block.atomic_neurons.front().leak.enable_multiplication;
	input_data.ports.set(
	    {chip_descriptor, 0}, grenade::vx::signal_flow::vertex::Chip::Parameterization(config));
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, topology, input_data);
		// Third run: expect no CapMem settling time due to non-CapMem change
		EXPECT_LE(timer.get_ms(), capmem_settling_time_ms);
		// Not too fast (may change)
		EXPECT_GE(timer.get_ms(), 5);
	}
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, topology, input_data);
		// Fourth run: expect no CapMem settling time due to non-CapMem change and even faster
		// construction due to equality of config
		EXPECT_LE(timer.get_ms(), 90);
	}
}

// disabled due to high performance variance on cluster
TEST(JITGraphExecutor, DISABLED_NoDifferentialConfig)
{
	auto topology = std::make_shared<grenade::common::Topology>();
	auto const chip_descriptor = topology->add_vertex(grenade::vx::signal_flow::vertex::Chip{
	    grenade::vx::common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(),
	    grenade::common::ExecutionInstanceOnExecutor(
	        grenade::common::ExecutionInstanceID(), grenade::common::ConnectionOnExecutor())});
	grenade::common::InputData input_data;
	lola::vx::v3::Chip config;
	input_data.ports.set(
	    {chip_descriptor, 0}, grenade::vx::signal_flow::vertex::Chip::Parameterization(config));

	// construct JIT executor with differential config mode disabled
	grenade::vx::execution::JITGraphExecutor executor(false);

	// a single batch entry with some runtime to ensure use of hardware
	input_data.time_domain_runtimes.set(
	    grenade::common::TimeDomainOnTopology(),
	    grenade::vx::network::abstract::ClockCycleTimeDomainRuntimes(
	        {grenade::vx::common::Time(100)}, grenade::vx::common::Time()));

	auto logger = log4cxx::Logger::getLogger("TEST_JITGraphExecutor.NoDifferentialConfig");
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, topology, input_data);
		// First run: expect CapMem settling time
		EXPECT_GE(timer.get_ms(), capmem_settling_time_ms);
	}
	// change CapMem cell value
	config.neuron_block.atomic_neurons.front().leak.v_leak =
	    lola::vx::v3::AtomicNeuron::AnalogValue(123);
	input_data.ports.set(
	    {chip_descriptor, 0}, grenade::vx::signal_flow::vertex::Chip::Parameterization(config));
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, topology, input_data);
		// Second run: expect CapMem settling time
		EXPECT_GE(timer.get_ms(), capmem_settling_time_ms);
	}
	// change non-CapMem value
	config.neuron_block.atomic_neurons.front().leak.enable_multiplication =
	    !config.neuron_block.atomic_neurons.front().leak.enable_multiplication;
	input_data.ports.set(
	    {chip_descriptor, 0}, grenade::vx::signal_flow::vertex::Chip::Parameterization(config));
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, topology, input_data);
		// Third run: expect CapMem settling time
		EXPECT_GE(timer.get_ms(), capmem_settling_time_ms);
	}
	{
		hate::Timer timer;
		grenade::vx::execution::run(executor, topology, input_data);
		// Fourth run: expect CapMem settling time
		EXPECT_GE(timer.get_ms(), capmem_settling_time_ms);
	}
}

// disabled due to high performance variance on cluster
TEST(JITGraphExecutor, DISABLED_ConcurrentUsage)
{
	auto topology = std::make_shared<grenade::common::Topology>();
	auto const chip_descriptor = topology->add_vertex(grenade::vx::signal_flow::vertex::Chip{
	    grenade::vx::common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(),
	    grenade::common::ExecutionInstanceOnExecutor(
	        grenade::common::ExecutionInstanceID(), grenade::common::ConnectionOnExecutor())});
	grenade::common::InputData input_data;
	input_data.ports.set(
	    {chip_descriptor, 0},
	    grenade::vx::signal_flow::vertex::Chip::Parameterization(lola::vx::v3::Chip()));

	// construct JIT executor with differential config mode enabled
	grenade::vx::execution::JITGraphExecutor executor(true);

	// a single batch entry with some runtime to ensure use of hardware
	input_data.time_domain_runtimes.set(
	    grenade::common::TimeDomainOnTopology(),
	    grenade::vx::network::abstract::ClockCycleTimeDomainRuntimes(
	        {grenade::vx::common::Time(
	            10000 * grenade::vx::common::Time::fpga_clock_cycles_per_us)},
	        grenade::vx::common::Time()));

	std::vector<std::future<grenade::common::OutputData>> results;
	constexpr size_t num_concurrent = 100;

	auto const run_func = [&]() -> grenade::common::OutputData {
		return grenade::vx::execution::run(executor, topology, input_data);
	};

	hate::Timer timer;
	for (size_t i = 0; i < num_concurrent; ++i) {
		results.push_back(std::async(std::launch::async, run_func));
	}

	for (size_t i = 0; i < num_concurrent; ++i) {
		auto const data = results.at(i).get();
		auto const& results_global =
		    dynamic_cast<grenade::vx::network::abstract::ExecutionInstanceGlobal const&>(
		        data.execution_instances.get(grenade::common::ExecutionInstanceOnExecutor(
		            grenade::common::ExecutionInstanceID(),
		            grenade::common::ConnectionOnExecutor())));
		// expect at least the realtime runtime as duration
		EXPECT_GE(
		    results_global.device_usage_duration.at(grenade::vx::common::ChipOnConnection()),
		    std::chrono::milliseconds(10));
	}

	// expect no more than seven times the realtime runtime as duration
	// this ensures, that differential mode works concurrently, since otherwise we would expect at
	// least 100ms per execution
	EXPECT_LE(timer.get_ms(), 7 * 10 * num_concurrent);
}
