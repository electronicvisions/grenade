#include <gtest/gtest.h>

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/types.h"
#include "grenade/vx/vertex.h"
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

TEST(Addition, Single)
{
	constexpr size_t size = 256;

	grenade::vx::vertex::ExternalInput external_input(grenade::vx::ConnectionType::DataInt8, size);

	grenade::vx::vertex::DataInput data_input(grenade::vx::ConnectionType::Int8, size);

	grenade::vx::vertex::Addition addition(size);

	grenade::vx::vertex::DataOutput data_output(grenade::vx::ConnectionType::Int8, size);

	grenade::vx::Graph g;

	grenade::vx::coordinate::ExecutionInstance instance;

	auto const v1 = g.add(external_input, instance, {});
	auto const v2 = g.add(data_input, instance, {v1});
	auto const v3 = g.add(addition, instance, {v2, v2});
	auto const v4 = g.add(data_output, instance, {v3});

	// Construct map of one connection and connect to HW
	grenade::vx::JITGraphExecutor executor;
	grenade::vx::backend::Connection connection;
	executor.acquire_connection(DLSGlobal(), std::move(connection));

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
	auto const result_map = executor.run(g, input_list, chip_configs);

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
		    static_cast<uint8_t>(
		        std::min(std::max(intmax_t(2 * i), intmax_t(-128)), intmax_t(127))),
		    static_cast<uint8_t>(
		        std::get<
		            std::vector<grenade::vx::TimedDataSequence<std::vector<grenade::vx::Int8>>>>(
		            result_map.data.at(v4))
		            .at(0)
		            .at(0)
		            .data.at(i + 128)));
	}
}
