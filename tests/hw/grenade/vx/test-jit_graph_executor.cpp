#include <gtest/gtest.h>

#include "grenade/vx/config.h"
#include "grenade/vx/data_map.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/jit_graph_executor.h"

TEST(JITGraphExecutor, Empty)
{
	grenade::vx::Graph g;

	grenade::vx::JITGraphExecutor::ExecutorMap executors;

	grenade::vx::DataMap input_list;

	grenade::vx::JITGraphExecutor::ConfigMap config_map;

	auto const result_map =
	    grenade::vx::JITGraphExecutor::run(g, input_list, executors, config_map);

	EXPECT_TRUE(result_map.empty());
}
