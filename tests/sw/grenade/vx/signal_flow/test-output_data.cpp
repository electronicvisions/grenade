#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/output_data.h"
#include "grenade/vx/signal_flow/port.h"

using namespace grenade::vx;
using namespace grenade::vx::common;
using namespace grenade::vx::signal_flow;

TEST(OutputData, General)
{
	OutputData map;

	EXPECT_TRUE(map.empty());
	EXPECT_TRUE(map.valid());

	auto const data = OutputData::Entry(std::vector<TimedDataSequence<std::vector<Int8>>>{
	    {{common::Time(), {Int8(1), Int8(2)}}}, {{common::Time(), {Int8(3), Int8(4)}}}});
	map.data[0] = data;

	EXPECT_FALSE(map.empty());
	EXPECT_EQ(map.batch_size(), 2);
	EXPECT_TRUE(map.valid());

	OutputData map_move(std::move(map));
	EXPECT_EQ(data, map_move.data.at(0));

	OutputData map_move_2 = std::move(map_move);
	EXPECT_EQ(data, map_move_2.data.at(0));

	map_move_2.clear();
	EXPECT_TRUE(map_move_2.empty());

	OutputData map_2;
	auto const data_1 = OutputData::Entry(std::vector<TimedDataSequence<std::vector<Int8>>>{
	    {{common::Time(), {Int8(5), Int8(6)}}}, {{common::Time(), {Int8(7), Int8(8)}}}});
	map_2.data[1] = data_1;
	map.merge(map_2);
	EXPECT_TRUE(map_2.empty());
	EXPECT_TRUE(map.data.contains(1));
	EXPECT_EQ(map.data.at(1), data_1);
}

TEST(OutputData, is_match)
{
	auto const data = std::vector<TimedDataSequence<std::vector<Int8>>>{
	    {{common::Time(), {Int8(5), Int8(6)}}}, {{common::Time(), {Int8(7), Int8(8)}}}};
	{
		Port port(2, ConnectionType::Int8);
		EXPECT_TRUE(OutputData::is_match(data, port));
	}
	{
		Port port(3, ConnectionType::Int8);
		EXPECT_FALSE(OutputData::is_match(data, port));
	}
	{
		Port port(2, ConnectionType::UInt5);
		EXPECT_FALSE(OutputData::is_match(data, port));
	}
	{
		Port port(2, ConnectionType::DataInt8);
		EXPECT_TRUE(OutputData::is_match(data, port));
	}
}
