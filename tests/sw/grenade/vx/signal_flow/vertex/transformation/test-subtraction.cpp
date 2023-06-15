#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/vertex/transformation/subtraction.h"

using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex::transformation;

TEST(Subtraction, General)
{
	EXPECT_NO_THROW(Subtraction(12, 123));

	Subtraction config(12, 123);

	EXPECT_EQ(config.inputs().size(), 12);
	EXPECT_EQ(config.inputs().front().size, 123);
	EXPECT_EQ(config.output().size, 123);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::Int8);
	EXPECT_EQ(config.output().type, ConnectionType::Int8);
}
