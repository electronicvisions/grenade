#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/vertex/argmax.h"

using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex;

TEST(ArgMax, General)
{
	EXPECT_NO_THROW(ArgMax(123, ConnectionType::Int8));
	EXPECT_THROW(ArgMax(123, ConnectionType::MembraneVoltage), std::runtime_error);

	ArgMax config(123, ConnectionType::Int8);

	EXPECT_EQ(config.inputs().size(), 1);
	EXPECT_EQ(config.inputs().front().size, 123);
	EXPECT_EQ(config.output().size, 1);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::Int8);
	EXPECT_EQ(config.output().type, ConnectionType::UInt32);
}
