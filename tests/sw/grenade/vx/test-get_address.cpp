#include <gtest/gtest.h>

#include "grenade/vx/helper.h"
#include "halco/common/iter_all.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v1;
using namespace grenade::vx;

TEST(get_address, Zero)
{
	for (auto const row : iter_all<SynapseRowOnSynram>()) {
		EXPECT_EQ(get_address(row, UInt5(0)), std::nullopt);
	}
}

TEST(get_address, LargerZero)
{
	for (auto const act : iter_all<UInt5>()) {
		if (act > 0) {
			for (auto const row : iter_all<SynapseRowOnSynram>()) {
				EXPECT_EQ(get_address(row, act), (31 - (act - 1)) + ((row.toEnum() % 2) ? 32 : 0));
			}
		}
	}

	EXPECT_EQ(get_address(SynapseRowOnSynram(), UInt5(1)), 31);
	EXPECT_EQ(get_address(SynapseRowOnSynram(), UInt5(31)), 1);
}
