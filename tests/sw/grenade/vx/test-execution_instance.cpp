#include <gtest/gtest.h>

#include "grenade/vx/execution_instance.h"

using namespace grenade::vx::coordinate;
using namespace halco::hicann_dls::vx;

TEST(ExecutionInstance, General)
{
	ExecutionInstance ei(ExecutionIndex(123), DLSGlobal(12));

	EXPECT_EQ(ei.toExecutionIndex(), ExecutionIndex(123));
	EXPECT_EQ(ei.toDLSGlobal(), DLSGlobal(12));

	std::stringstream ss;
	ss << ei;
	EXPECT_EQ(ss.str(), "ExecutionInstance(ExecutionIndex(123), DLSGlobal(12))");

	{
		ExecutionInstance ei_other(ExecutionIndex(122), DLSGlobal(12));
		EXPECT_NE(ei, ei_other);
	}
	{
		ExecutionInstance ei_other(ExecutionIndex(123), DLSGlobal(11));
		EXPECT_NE(ei, ei_other);
	}
	ExecutionInstance ei_copy = ei;
	EXPECT_EQ(ei, ei_copy);
}
