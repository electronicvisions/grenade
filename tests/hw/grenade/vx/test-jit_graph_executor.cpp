#include <gtest/gtest.h>

#include "grenade/vx/graph.h"
#include "grenade/vx/io_data_list.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"
#include "lola/vx/v2/chip.h"

TEST(JITGraphExecutor, Empty)
{
	grenade::vx::Graph g;

	grenade::vx::JITGraphExecutor::Connections connections;

	grenade::vx::IODataMap input_map;

	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;

	auto const result_map =
	    grenade::vx::JITGraphExecutor::run(g, input_map, connections, chip_configs);
	EXPECT_TRUE(result_map.empty());

	grenade::vx::IODataList input_list;
	input_list.from_input_map(input_map, g);

	auto const result_list =
	    grenade::vx::JITGraphExecutor::run(g, input_list, connections, chip_configs);
	EXPECT_TRUE(result_list.data.empty());
}
