#include <gtest/gtest.h>

#include "grenade/vx/vertex/relu.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

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
