#include <gtest/gtest.h>

#include "grenade/vx/vertex/subtraction.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(Subtraction, General)
{
	EXPECT_NO_THROW(Subtraction(123));

	Subtraction config(123);

	EXPECT_TRUE(config.variadic_input);
	EXPECT_EQ(config.inputs().size(), 1);
	EXPECT_EQ(config.inputs().front().size, 123);
	EXPECT_EQ(config.output().size, 123);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::Int8);
	EXPECT_EQ(config.output().type, ConnectionType::Int8);
}
