#include <gtest/gtest.h>

#include "grenade/vx/vertex/converting_relu.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(ConvertingReLU, General)
{
	EXPECT_NO_THROW(ConvertingReLU(123, 2));

	ConvertingReLU config(123, 1);

	EXPECT_EQ(config.inputs().size(), 1);
	EXPECT_EQ(config.inputs().front().size, 123);
	EXPECT_EQ(config.output().size, 123);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::Int8);
	EXPECT_EQ(config.output().type, ConnectionType::UInt5);

	EXPECT_EQ(config.get_shift(), 1);
}
