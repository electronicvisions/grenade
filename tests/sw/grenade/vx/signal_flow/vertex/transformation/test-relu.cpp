#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/vertex/transformation/relu.h"

using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex::transformation;

TEST(ReLU, General)
{
	EXPECT_NO_THROW(ReLU(123));

	ReLU config(123);

	EXPECT_EQ(config.inputs().size(), 1);
	EXPECT_EQ(config.inputs().front().size, 123);
	EXPECT_EQ(config.output().size, 123);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::Int8);
	EXPECT_EQ(config.output().type, ConnectionType::Int8);
}
