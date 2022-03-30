#include <gtest/gtest.h>

#include "grenade/vx/connection_type.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/port.h"

using namespace grenade::vx;

TEST(IODataMap, General)
{
	IODataMap map;

	EXPECT_TRUE(map.empty());
	EXPECT_TRUE(map.valid());

	auto const data = IODataMap::Entry(std::vector<TimedDataSequence<std::vector<Int8>>>{
	    {{haldls::vx::v3::FPGATime(), haldls::vx::v3::ChipTime(), {Int8(1), Int8(2)}}},
	    {{haldls::vx::v3::FPGATime(), haldls::vx::v3::ChipTime(), {Int8(3), Int8(4)}}}});
	map.data[0] = data;

	EXPECT_FALSE(map.empty());
	EXPECT_EQ(map.batch_size(), 2);
	EXPECT_TRUE(map.valid());

	map.runtime[coordinate::ExecutionInstance()] = std::vector{haldls::vx::v3::Timer::Value(0)};
	EXPECT_FALSE(map.valid());
	EXPECT_THROW(map.batch_size(), std::runtime_error);
	std::unordered_map<coordinate::ExecutionInstance, std::vector<haldls::vx::v3::Timer::Value>>
	    runtime;
	runtime[coordinate::ExecutionInstance()] =
	    std::vector{haldls::vx::v3::Timer::Value(0), haldls::vx::v3::Timer::Value(1)};
	map.runtime = runtime;
	EXPECT_TRUE(map.valid());
	EXPECT_EQ(map.batch_size(), 2);

	IODataMap map_move(std::move(map));
	EXPECT_EQ(data, map_move.data.at(0));
	EXPECT_EQ(runtime, map_move.runtime);

	IODataMap map_move_2 = std::move(map_move);
	EXPECT_EQ(data, map_move_2.data.at(0));
	EXPECT_EQ(runtime, map_move_2.runtime);

	map_move_2.clear();
	EXPECT_TRUE(map_move_2.empty());

	IODataMap map_2;
	auto const data_1 = IODataMap::Entry(std::vector<TimedDataSequence<std::vector<Int8>>>{
	    {{haldls::vx::v3::FPGATime(), haldls::vx::v3::ChipTime(), {Int8(5), Int8(6)}}},
	    {{haldls::vx::v3::FPGATime(), haldls::vx::v3::ChipTime(), {Int8(7), Int8(8)}}}});
	map_2.data[1] = data_1;
	map.runtime = runtime;
	map.merge(map_2);
	EXPECT_TRUE(map_2.empty());
	EXPECT_TRUE(map.data.contains(1));
	EXPECT_EQ(map.data.at(1), data_1);
	EXPECT_EQ(map.runtime, runtime);
	std::unordered_map<coordinate::ExecutionInstance, std::vector<haldls::vx::v3::Timer::Value>>
	    runtime_2;
	runtime_2[coordinate::ExecutionInstance()] = std::vector{haldls::vx::v3::Timer::Value(0)};
	map_2.runtime = runtime_2;
	map.merge(map_2);
	EXPECT_EQ(map.runtime, runtime);
	map.runtime.clear();
	map.merge(map_2);
	EXPECT_EQ(map.runtime, runtime_2);
}

TEST(IODataMap, is_match)
{
	auto const data = std::vector<TimedDataSequence<std::vector<Int8>>>{
	    {{haldls::vx::v3::FPGATime(), haldls::vx::v3::ChipTime(), {Int8(5), Int8(6)}}},
	    {{haldls::vx::v3::FPGATime(), haldls::vx::v3::ChipTime(), {Int8(7), Int8(8)}}}};
	{
		Port port(2, ConnectionType::Int8);
		EXPECT_TRUE(IODataMap::is_match(data, port));
	}
	{
		Port port(3, ConnectionType::Int8);
		EXPECT_FALSE(IODataMap::is_match(data, port));
	}
	{
		Port port(2, ConnectionType::UInt5);
		EXPECT_FALSE(IODataMap::is_match(data, port));
	}
	{
		Port port(2, ConnectionType::DataInt8);
		EXPECT_TRUE(IODataMap::is_match(data, port));
	}
}
