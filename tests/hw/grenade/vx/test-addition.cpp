#include <gtest/gtest.h>

#include "grenade/vx/config.h"
#include "grenade/vx/data_map.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/types.h"
#include "grenade/vx/vertex.h"
#include "halco/hicann-dls/vx/chip.h"
#include "haldls/vx/systime.h"
#include "hxcomm/vx/connection_from_env.h"
#include "logging_ctrl.h"
#include "stadls/vx/init_generator.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/run.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx;
using namespace stadls::vx;
using namespace lola::vx;

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

	grenade::vx::coordinate::ExecutionInstance instance(
	    grenade::vx::coordinate::ExecutionIndex(0), HemisphereGlobal());

	auto const v1 = g.add(external_input, instance, {});
	auto const v2 = g.add(data_input, instance, {v1});
	auto const v3 = g.add(addition, instance, {v2, v2});
	auto const v4 = g.add(data_output, instance, {v3});

	// Construct map of one executor and connect to HW
	grenade::vx::JITGraphExecutor::ExecutorMap executors;
	auto connection = hxcomm::vx::get_connection_from_env();
	executors.insert(std::pair<DLSGlobal, hxcomm::vx::ConnectionVariant&>(DLSGlobal(), connection));

	// Initialize chip
	{
		DigitalInit const init;
		auto [builder, _] = generate(init);
		auto program = builder.done();
		stadls::vx::run(executors.at(DLSGlobal()), program);
	}

	// fill graph inputs (with UInt5(0))
	grenade::vx::DataMap input_list;
	std::vector<grenade::vx::Int8> inputs(size);
	for (intmax_t i = -128; i < 127; ++i) {
		inputs.at(i + 128) = grenade::vx::Int8(i);
	}
	input_list.int8.insert(std::make_pair(v1, inputs));

	std::unique_ptr<grenade::vx::ChipConfig> chip = std::make_unique<grenade::vx::ChipConfig>();
	grenade::vx::JITGraphExecutor::ConfigMap config_map;
	config_map[DLSGlobal()] = *chip;

	// run Graph with given inputs and return results
	auto const output_activation_map =
	    grenade::vx::JITGraphExecutor::run(g, input_list, executors, config_map);

	EXPECT_EQ(output_activation_map.int8.size(), 1);

	EXPECT_TRUE(output_activation_map.int8.find(v4) != output_activation_map.int8.end());
	EXPECT_EQ(output_activation_map.int8.at(v4).size(), size);
	for (intmax_t i = -128; i < 127; ++i) {
		EXPECT_EQ(
		    static_cast<uint8_t>(
		        std::min(std::max(intmax_t(2 * i), intmax_t(-128)), intmax_t(127))),
		    static_cast<uint8_t>(output_activation_map.int8.at(v4).at(i + 128)));
	}
}
