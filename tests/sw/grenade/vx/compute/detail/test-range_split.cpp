#include <gtest/gtest.h>

#include "grenade/vx/compute/detail/range_split.h"

using namespace grenade::vx::compute::detail;

TEST(RangeSplit, General)
{
	RangeSplit const splitter(12);

	auto const ranges = splitter(50);
	EXPECT_EQ(ranges.size(), 5);
	EXPECT_EQ(ranges.at(0).size, 12);
	EXPECT_EQ(ranges.at(0).offset, 0);
	EXPECT_EQ(ranges.at(1).size, 12);
	EXPECT_EQ(ranges.at(1).offset, 12);
	EXPECT_EQ(ranges.at(2).size, 12);
	EXPECT_EQ(ranges.at(2).offset, 24);
	EXPECT_EQ(ranges.at(3).size, 12);
	EXPECT_EQ(ranges.at(3).offset, 36);
	EXPECT_EQ(ranges.at(4).size, 2);
	EXPECT_EQ(ranges.at(4).offset, 48);

	auto const ranges_s = splitter(10);
	EXPECT_EQ(ranges_s.size(), 1);
	EXPECT_EQ(ranges_s.at(0).size, 10);
	EXPECT_EQ(ranges_s.at(0).offset, 0);
}
