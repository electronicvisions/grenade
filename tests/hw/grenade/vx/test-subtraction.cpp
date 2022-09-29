#include <gtest/gtest.h>

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/signal_flow/execution_instance.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input.h"
#include "grenade/vx/signal_flow/vertex.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/systime.h"
#include "hxcomm/vx/connection_from_env.h"
#include "logging_ctrl.h"
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

	grenade::vx::signal_flow::vertex::ExternalInput external_input(
	    grenade::vx::signal_flow::ConnectionType::DataInt8, size);

	grenade::vx::signal_flow::vertex::DataInput data_input(
	    grenade::vx::signal_flow::ConnectionType::Int8, size);

	grenade::vx::signal_flow::vertex::Subtraction subtraction(size);

	grenade::vx::signal_flow::vertex::DataOutput data_output(
	    grenade::vx::signal_flow::ConnectionType::Int8, size);

	grenade::vx::signal_flow::Graph g;

	grenade::vx::signal_flow::ExecutionInstance instance;

	auto const v1 = g.add(external_input, instance, {});
	auto const v2 = g.add(data_input, instance, {v1});
	auto const v3 = g.add(subtraction, instance, {v2, v2});
	auto const v4 = g.add(data_output, instance, {v3});

	// Construct map of one connection and connect to HW
	grenade::vx::backend::Connection connection;
	std::map<DLSGlobal, grenade::vx::backend::Connection> connections;
	connections.emplace(DLSGlobal(), std::move(connection));
	grenade::vx::JITGraphExecutor executor(std::move(connections));

	// fill graph inputs
	grenade::vx::IODataMap input_list;
	std::vector<grenade::vx::Int8> inputs(size);
	for (intmax_t i = -128; i < 127; ++i) {
		inputs.at(i + 128) = grenade::vx::Int8(i);
	}
	input_list.data[v1] =
	    std::vector<grenade::vx::TimedDataSequence<std::vector<grenade::vx::Int8>>>{
	        {{haldls::vx::v3::FPGATime(), haldls::vx::v3::ChipTime(), inputs}}};

	std::unique_ptr<lola::vx::v3::Chip> chip = std::make_unique<lola::vx::v3::Chip>();
	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[instance] = *chip;

	// run Graph with given inputs and return results
	auto const result_map = grenade::vx::run(executor, g, input_list, chip_configs);

	EXPECT_EQ(result_map.data.size(), 1);

	EXPECT_TRUE(result_map.data.find(v4) != result_map.data.end());
	EXPECT_EQ(
	    std::get<std::vector<grenade::vx::TimedDataSequence<std::vector<grenade::vx::Int8>>>>(
	        result_map.data.at(v4))
	        .size(),
	    1);
	EXPECT_EQ(
	    std::get<std::vector<grenade::vx::TimedDataSequence<std::vector<grenade::vx::Int8>>>>(
	        result_map.data.at(v4))
	        .at(0)
	        .size(),
	    1);
	EXPECT_EQ(
	    std::get<std::vector<grenade::vx::TimedDataSequence<std::vector<grenade::vx::Int8>>>>(
	        result_map.data.at(v4))
	        .at(0)
	        .at(0)
	        .data.size(),
	    size);
	for (intmax_t i = -128; i < 127; ++i) {
		EXPECT_EQ(
		    0,
		    std::get<std::vector<grenade::vx::TimedDataSequence<std::vector<grenade::vx::Int8>>>>(
		        result_map.data.at(v4))
		        .at(0)
		        .at(0)
		        .data.at(i + 128)
		        .value());
	}
}
