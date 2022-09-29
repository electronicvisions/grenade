#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"

using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex;

TEST(CrossbarL2Output, General)
{
	CrossbarL2Output config;

	EXPECT_TRUE(config.variadic_input);
	EXPECT_EQ(config.inputs().size(), 1);
	EXPECT_EQ(config.inputs().front().size, 1);
	EXPECT_EQ(config.output().size, 1);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::CrossbarOutputLabel);
	EXPECT_EQ(config.output().type, ConnectionType::TimedSpikeFromChipSequence);
}
