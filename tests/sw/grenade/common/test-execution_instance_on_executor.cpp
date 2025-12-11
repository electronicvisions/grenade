#include "grenade/common/execution_instance_on_executor.h"

#include <gtest/gtest.h>

TEST(ExecutionInstanceOnExecutor, General)
{
	using namespace grenade::common;

	ExecutionInstanceOnExecutor value_0_1(ExecutionInstanceID(0), ConnectionOnExecutor(1));
	ExecutionInstanceOnExecutor value_0_2(ExecutionInstanceID(0), ConnectionOnExecutor(2));
	ExecutionInstanceOnExecutor value_1_1(ExecutionInstanceID(1), ConnectionOnExecutor(1));

	EXPECT_EQ(value_0_1, value_0_1);

	EXPECT_NE(value_0_1, value_1_1);
	EXPECT_NE(value_0_1, value_0_2);

	EXPECT_LE(value_0_1, value_0_1);
	EXPECT_LE(value_0_1, value_0_2);
	EXPECT_LE(value_0_1, value_1_1);

	EXPECT_LT(value_0_1, value_0_2);
	EXPECT_LT(value_0_1, value_1_1);

	EXPECT_GE(value_0_1, value_0_1);
	EXPECT_GE(value_0_2, value_0_1);
	EXPECT_GE(value_1_1, value_0_1);

	EXPECT_GT(value_0_2, value_0_1);
	EXPECT_GT(value_1_1, value_0_1);
}
