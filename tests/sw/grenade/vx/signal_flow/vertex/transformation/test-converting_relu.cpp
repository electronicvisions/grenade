#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/vertex/transformation/converting_relu.h"

using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex::transformation;

TEST(ConvertingReLU, General)
{
	EXPECT_NO_THROW(ConvertingReLU(123, 2));

	ConvertingReLU config(123, 1);

	EXPECT_EQ(config.inputs().size(), 1);
	EXPECT_EQ(config.inputs().front().size, 123);
	EXPECT_EQ(config.output().size, 123);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::Int8);
	EXPECT_EQ(config.output().type, ConnectionType::UInt5);
}
