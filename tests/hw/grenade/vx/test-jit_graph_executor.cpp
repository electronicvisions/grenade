#include <gtest/gtest.h>

#include "grenade/vx/graph.h"
#include "grenade/vx/io_data_list.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"

TEST(JITGraphExecutor, Empty)
{
	grenade::vx::Graph g;

	grenade::vx::JITGraphExecutor executor;

	grenade::vx::JITGraphExecutor::ChipConfigs initial_config;

	grenade::vx::IODataMap input_map;

	auto const result_map = executor.run(g, input_map, initial_config);
	EXPECT_TRUE(result_map.empty());

	grenade::vx::IODataList input_list;
	input_list.from_input_map(input_map, g);

	auto const result_list = executor.run(g, input_list, initial_config);
	EXPECT_TRUE(result_list.data.empty());
}
