#include <gtest/gtest.h>

#include "grenade/vx/config.h"
#include "grenade/vx/data_map.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/types.h"
#include "grenade/vx/vertex.h"
#include "halco/hicann-dls/vx/v2/chip.h"
#include "haldls/vx/v2/systime.h"
#include "hxcomm/vx/connection_from_env.h"
#include "logging_ctrl.h"
#include "stadls/vx/v2/init_generator.h"
#include "stadls/vx/v2/playback_generator.h"
#include "stadls/vx/v2/run.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v2;
using namespace stadls::vx::v2;
using namespace lola::vx::v2;

TEST(Addition, Single)
{
	logger_default_config(log4cxx::Level::getTrace());

	constexpr size_t size = 256;

	grenade::vx::vertex::ExternalInput external_input(
	    grenade::vx::ConnectionType::DataOutputInt8, size);

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
	grenade::vx::JITGraphExecutor::Connections connections;
	auto connection = hxcomm::vx::get_connection_from_env();
	connections.insert(
	    std::pair<DLSGlobal, hxcomm::vx::ConnectionVariant&>(DLSGlobal(), connection));

	// Initialize chip
	{
		DigitalInit const init;
		auto [builder, _] = generate(init);
		auto program = builder.done();
		stadls::vx::v2::run(connections.at(DLSGlobal()), program);
	}

	// fill graph inputs
	grenade::vx::DataMap input_list;
	std::vector<grenade::vx::Int8> inputs(size);
	for (intmax_t i = -128; i < 127; ++i) {
		inputs.at(i + 128) = grenade::vx::Int8(i);
	}
	input_list.int8[v1] = {inputs};

	std::unique_ptr<grenade::vx::ChipConfig> chip = std::make_unique<grenade::vx::ChipConfig>();
	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[DLSGlobal()] = *chip;

	// run Graph with given inputs and return results
	auto const result_map =
	    grenade::vx::JITGraphExecutor::run(g, input_list, connections, chip_configs);

	EXPECT_EQ(result_map.int8.size(), 1);

	EXPECT_TRUE(result_map.int8.find(v4) != result_map.int8.end());
	EXPECT_EQ(result_map.int8.at(v4).size(), 1);
	EXPECT_EQ(result_map.int8.at(v4).at(0).size(), size);
	for (intmax_t i = -128; i < 127; ++i) {
		EXPECT_EQ(
		    static_cast<uint8_t>(
		        std::min(std::max(intmax_t(2 * i), intmax_t(-128)), intmax_t(127))),
		    static_cast<uint8_t>(result_map.int8.at(v4).at(0).at(i + 128)));
	}
}
