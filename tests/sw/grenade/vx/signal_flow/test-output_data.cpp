#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/output_data.h"

using namespace grenade::vx;
using namespace grenade::vx::common;
using namespace grenade::vx::signal_flow;

TEST(OutputData, General)
{
	OutputData map;

	EXPECT_TRUE(map.empty());
	EXPECT_TRUE(map.valid());

	map.snippets.resize(1);

	EXPECT_TRUE(map.empty());
	EXPECT_TRUE(map.valid());

	auto const data = OutputDataSnippet::Entry(std::vector<TimedDataSequence<std::vector<Int8>>>{
	    {{common::Time(), {Int8(1), Int8(2)}}}, {{common::Time(), {Int8(3), Int8(4)}}}});
	map.snippets.at(0).data[0] = data;

	EXPECT_FALSE(map.empty());
	EXPECT_EQ(map.batch_size(), 2);
	EXPECT_TRUE(map.valid());

	OutputData map_move(std::move(map));
	EXPECT_EQ(data, map_move.snippets.at(0).data.at(0));

	OutputData map_move_2 = std::move(map_move);
	EXPECT_EQ(data, map_move_2.snippets.at(0).data.at(0));

	map_move_2.clear();
	EXPECT_TRUE(map_move_2.empty());

	OutputData map_2;
	map_2.snippets.resize(1);
	auto const data_1 = OutputDataSnippet::Entry(std::vector<TimedDataSequence<std::vector<Int8>>>{
	    {{common::Time(), {Int8(5), Int8(6)}}}, {{common::Time(), {Int8(7), Int8(8)}}}});
	map_2.snippets.at(0).data[1] = data_1;
	map.merge(map_2);
	EXPECT_TRUE(map_2.empty());
	EXPECT_TRUE(map.snippets.at(0).data.contains(1));
	EXPECT_EQ(map.snippets.at(0).data.at(1), data_1);
}
