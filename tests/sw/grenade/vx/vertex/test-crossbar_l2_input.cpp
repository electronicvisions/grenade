#include <gtest/gtest.h>

#include "grenade/vx/vertex/crossbar_l2_input.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(CrossbarL2Input, General)
{
	CrossbarL2Input config;

	EXPECT_FALSE(config.variadic_input);
	EXPECT_EQ(config.inputs().size(), 1);
	EXPECT_EQ(config.inputs().front().size, 1);
	EXPECT_EQ(config.output().size, 1);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::TimedSpikeSequence);
	EXPECT_EQ(config.output().type, ConnectionType::CrossbarInputLabel);
}
