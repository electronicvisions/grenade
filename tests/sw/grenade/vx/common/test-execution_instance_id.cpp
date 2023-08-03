#include <gtest/gtest.h>

#include "grenade/vx/common/execution_instance_id.h"

using namespace grenade::vx::common;
using namespace halco::hicann_dls::vx;

TEST(ExecutionInstanceID, General)
{
	ExecutionInstanceID ei(ExecutionIndex(123), DLSGlobal(12));

	EXPECT_EQ(ei.toExecutionIndex(), ExecutionIndex(123));
	EXPECT_EQ(ei.toDLSGlobal(), DLSGlobal(12));

	std::stringstream ss;
	ss << ei;
	EXPECT_EQ(ss.str(), "ExecutionInstanceID(ExecutionIndex(123), DLSGlobal(12))");

	{
		ExecutionInstanceID ei_other(ExecutionIndex(122), DLSGlobal(12));
		EXPECT_NE(ei, ei_other);
	}
	{
		ExecutionInstanceID ei_other(ExecutionIndex(123), DLSGlobal(11));
		EXPECT_NE(ei, ei_other);
	}
	ExecutionInstanceID ei_copy = ei;
	EXPECT_EQ(ei, ei_copy);
}
