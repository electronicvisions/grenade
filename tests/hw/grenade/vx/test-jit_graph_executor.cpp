#include <gtest/gtest.h>

#include "grenade/vx/config.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"

TEST(JITGraphExecutor, Empty)
{
	grenade::vx::Graph g;

	grenade::vx::JITGraphExecutor::Connections connections;

	grenade::vx::IODataMap input_list;

	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;

	auto const result_map =
	    grenade::vx::JITGraphExecutor::run(g, input_list, connections, chip_configs);

	EXPECT_TRUE(result_map.empty());
}
