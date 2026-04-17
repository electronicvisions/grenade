#include <gtest/gtest.h>

#include "grenade/common/execution_instance_id.h"
#include "grenade/common/input_data.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/topology.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/transformation/subtraction.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/systime.h"
#include "hxcomm/vx/connection_from_env.h"
#include "lola/vx/v3/chip.h"
#include "stadls/vx/v3/init_generator.h"
#include "stadls/vx/v3/playback_generator.h"
#include "stadls/vx/v3/run.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace stadls::vx::v3;
using namespace lola::vx::v3;

TEST(Subtraction, Single)
{
	constexpr size_t size = 256;

	grenade::vx::signal_flow::vertex::Transformation subtraction(
	    grenade::vx::signal_flow::vertex::transformation::Subtraction(2, size),
	    grenade::common::ExecutionInstanceOnExecutor(
	        grenade::common::ExecutionInstanceID(1), grenade::common::ConnectionOnExecutor()));

	auto topology = std::make_shared<grenade::common::Topology>();

	auto const v1 = topology->add_vertex(subtraction);

	// Construct map of one connection and connect to HW
	grenade::vx::execution::JITGraphExecutor executor;

	// fill inputs
	grenade::common::InputData input_data;
	std::vector<grenade::vx::signal_flow::Int8> inputs(size);
	for (intmax_t i = -128; i < 127; ++i) {
		inputs.at(i + 128) = grenade::vx::signal_flow::Int8(i);
	}
	input_data.ports.set(
	    {v1, 0}, grenade::vx::signal_flow::vertex::Transformation::Dynamics(
	                 std::vector<grenade::vx::common::TimedDataSequence<
	                     std::vector<grenade::vx::signal_flow::Int8>>>{
	                     {{grenade::vx::common::Time(), inputs}}}));
	input_data.ports.set(
	    {v1, 1}, grenade::vx::signal_flow::vertex::Transformation::Dynamics(
	                 std::vector<grenade::vx::common::TimedDataSequence<
	                     std::vector<grenade::vx::signal_flow::Int8>>>{
	                     {{grenade::vx::common::Time(), inputs}}}));

	// run Graph with given inputs and return results
	auto const results = grenade::vx::execution::run(executor, topology, input_data);

	EXPECT_EQ(results.batch_size(), 1);

	EXPECT_TRUE(results.ports.contains({v1, 0}));
	auto const& result =
	    dynamic_cast<grenade::vx::signal_flow::vertex::Transformation::Results const&>(
	        results.ports.get({v1, 0}));
	EXPECT_EQ(
	    std::get<std::vector<
	        grenade::vx::common::TimedDataSequence<std::vector<grenade::vx::signal_flow::Int8>>>>(
	        result.value)
	        .size(),
	    1);
	EXPECT_EQ(
	    std::get<std::vector<
	        grenade::vx::common::TimedDataSequence<std::vector<grenade::vx::signal_flow::Int8>>>>(
	        result.value)
	        .at(0)
	        .at(0)
	        .data.size(),
	    size);
	for (intmax_t i = -128; i < 127; ++i) {
		EXPECT_EQ(
		    0, static_cast<uint8_t>(std::get<std::vector<grenade::vx::common::TimedDataSequence<
		                                std::vector<grenade::vx::signal_flow::Int8>>>>(result.value)
		                                .at(0)
		                                .at(0)
		                                .data.at(i + 128)));
	}
}
