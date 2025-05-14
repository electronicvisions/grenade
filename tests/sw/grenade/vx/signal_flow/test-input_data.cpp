#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/input_data.h"

using namespace grenade::vx;
using namespace grenade::vx::common;
using namespace grenade::vx::signal_flow;

TEST(InputData, General)
{
	InputData map;

	EXPECT_TRUE(map.empty());
	EXPECT_TRUE(map.valid());

	map.snippets.resize(1);

	EXPECT_TRUE(map.empty());
	EXPECT_TRUE(map.valid());

	auto const data = InputDataSnippet::Entry(std::vector<TimedDataSequence<std::vector<Int8>>>{
	    {{common::Time(), {Int8(1), Int8(2)}}}, {{common::Time(), {Int8(3), Int8(4)}}}});
	map.snippets.at(0).data[0] = data;

	EXPECT_FALSE(map.empty());
	EXPECT_EQ(map.batch_size(), 2);
	EXPECT_TRUE(map.valid());

	map.snippets.at(0).runtime.resize(1);
	map.snippets.at(0).runtime.at(0)[grenade::common::ExecutionInstanceID()] = common::Time(0);
	EXPECT_FALSE(map.valid());
	EXPECT_THROW(map.batch_size(), std::runtime_error);
	std::vector<std::map<grenade::common::ExecutionInstanceID, common::Time>> runtime{
	    {{grenade::common::ExecutionInstanceID(), common::Time(0)}},
	    {{grenade::common::ExecutionInstanceID(), common::Time(1)}}};
	map.snippets.at(0).runtime = runtime;
	EXPECT_TRUE(map.valid());
	EXPECT_EQ(map.batch_size(), 2);

	InputData map_move(std::move(map));
	EXPECT_EQ(data, map_move.snippets.at(0).data.at(0));
	EXPECT_EQ(runtime, map_move.snippets.at(0).runtime);

	InputData map_move_2 = std::move(map_move);
	EXPECT_EQ(data, map_move_2.snippets.at(0).data.at(0));
	EXPECT_EQ(runtime, map_move_2.snippets.at(0).runtime);

	map_move_2.clear();
	EXPECT_TRUE(map_move_2.empty());

	InputData map_2;
	map_2.snippets.resize(1);
	auto const data_1 = InputDataSnippet::Entry(std::vector<TimedDataSequence<std::vector<Int8>>>{
	    {{common::Time(), {Int8(5), Int8(6)}}}, {{common::Time(), {Int8(7), Int8(8)}}}});
	map_2.snippets.at(0).data[1] = data_1;
	map_2.snippets.at(0).runtime = runtime;
	map.merge(map_2);
	EXPECT_TRUE(map_2.empty());
	EXPECT_TRUE(map.snippets.at(0).data.contains(1));
	EXPECT_EQ(map.snippets.at(0).data.at(1), data_1);
	EXPECT_EQ(map.snippets.at(0).runtime, runtime);
	std::vector<std::map<grenade::common::ExecutionInstanceID, common::Time>> runtime_2{
	    {{grenade::common::ExecutionInstanceID(), common::Time(0)}},
	    {{grenade::common::ExecutionInstanceID(), common::Time(1)}}};
	map_2.snippets.at(0).runtime = runtime_2;
	map.merge(map_2);
	EXPECT_EQ(map.snippets.at(0).runtime, runtime);
	map.snippets.at(0).runtime.clear();
	map.merge(map_2);
	EXPECT_EQ(map.snippets.at(0).runtime, runtime_2);
}
