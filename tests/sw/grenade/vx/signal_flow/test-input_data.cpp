#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/input_data.h"
#include "grenade/vx/signal_flow/port.h"

using namespace grenade::vx;
using namespace grenade::vx::common;
using namespace grenade::vx::signal_flow;

TEST(InputData, General)
{
	InputData map;

	EXPECT_TRUE(map.empty());
	EXPECT_TRUE(map.valid());

	auto const data = InputData::Entry(std::vector<TimedDataSequence<std::vector<Int8>>>{
	    {{common::Time(), {Int8(1), Int8(2)}}}, {{common::Time(), {Int8(3), Int8(4)}}}});
	map.data[0] = data;

	EXPECT_FALSE(map.empty());
	EXPECT_EQ(map.batch_size(), 2);
	EXPECT_TRUE(map.valid());

	map.runtime.resize(1);
	map.runtime.at(0)[ExecutionInstanceID()] = common::Time(0);
	EXPECT_FALSE(map.valid());
	EXPECT_THROW(map.batch_size(), std::runtime_error);
	std::vector<std::map<ExecutionInstanceID, common::Time>> runtime{
	    {{ExecutionInstanceID(), common::Time(0)}}, {{ExecutionInstanceID(), common::Time(1)}}};
	map.runtime = runtime;
	EXPECT_TRUE(map.valid());
	EXPECT_EQ(map.batch_size(), 2);

	InputData map_move(std::move(map));
	EXPECT_EQ(data, map_move.data.at(0));
	EXPECT_EQ(runtime, map_move.runtime);

	InputData map_move_2 = std::move(map_move);
	EXPECT_EQ(data, map_move_2.data.at(0));
	EXPECT_EQ(runtime, map_move_2.runtime);

	map_move_2.clear();
	EXPECT_TRUE(map_move_2.empty());

	InputData map_2;
	auto const data_1 = InputData::Entry(std::vector<TimedDataSequence<std::vector<Int8>>>{
	    {{common::Time(), {Int8(5), Int8(6)}}}, {{common::Time(), {Int8(7), Int8(8)}}}});
	map_2.data[1] = data_1;
	map_2.runtime = runtime;
	map.merge(map_2);
	EXPECT_TRUE(map_2.empty());
	EXPECT_TRUE(map.data.contains(1));
	EXPECT_EQ(map.data.at(1), data_1);
	EXPECT_EQ(map.runtime, runtime);
	std::vector<std::map<ExecutionInstanceID, common::Time>> runtime_2{
	    {{ExecutionInstanceID(), common::Time(0)}}, {{ExecutionInstanceID(), common::Time(1)}}};
	map_2.runtime = runtime_2;
	map.merge(map_2);
	EXPECT_EQ(map.runtime, runtime);
	map.runtime.clear();
	map.merge(map_2);
	EXPECT_EQ(map.runtime, runtime_2);
}

TEST(InputData, is_match)
{
	auto const data = std::vector<TimedDataSequence<std::vector<Int8>>>{
	    {{common::Time(), {Int8(5), Int8(6)}}}, {{common::Time(), {Int8(7), Int8(8)}}}};
	{
		Port port(2, ConnectionType::Int8);
		EXPECT_TRUE(InputData::is_match(data, port));
	}
	{
		Port port(3, ConnectionType::Int8);
		EXPECT_FALSE(InputData::is_match(data, port));
	}
	{
		Port port(2, ConnectionType::UInt5);
		EXPECT_FALSE(InputData::is_match(data, port));
	}
	{
		Port port(2, ConnectionType::DataInt8);
		EXPECT_TRUE(InputData::is_match(data, port));
	}
}
