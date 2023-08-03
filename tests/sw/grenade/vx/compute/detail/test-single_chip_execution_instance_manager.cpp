#include <gtest/gtest.h>

#include "grenade/vx/compute/detail/single_chip_execution_instance_manager.h"

using namespace halco::hicann_dls::vx::v3;

TEST(SingleChipExecutionInstanceManager, General)
{
	grenade::vx::compute::detail::SingleChipExecutionInstanceManager manager;

	EXPECT_EQ(manager.get_current_hemisphere(), HemisphereOnDLS(0));

	{
		auto const ei = manager.next();
		EXPECT_EQ(ei, grenade::vx::common::ExecutionInstanceID());
		EXPECT_EQ(manager.get_current_hemisphere(), HemisphereOnDLS(1));
	}
	{
		auto const ei = manager.next();
		EXPECT_EQ(
		    ei, grenade::vx::common::ExecutionInstanceID(
		            grenade::vx::common::ExecutionIndex(1), DLSGlobal()));
		EXPECT_EQ(manager.get_current_hemisphere(), HemisphereOnDLS(0));
	}
	{
		auto const ei = manager.next();
		EXPECT_EQ(
		    ei, grenade::vx::common::ExecutionInstanceID(
		            grenade::vx::common::ExecutionIndex(1), DLSGlobal()));
		EXPECT_EQ(manager.get_current_hemisphere(), HemisphereOnDLS(1));
	}
	{
		auto const ei = manager.next_index();
		EXPECT_EQ(
		    ei, grenade::vx::common::ExecutionInstanceID(
		            grenade::vx::common::ExecutionIndex(2), DLSGlobal()));
		EXPECT_EQ(manager.get_current_hemisphere(), HemisphereOnDLS(0));
	}
	{
		auto const ei = manager.next_index();
		EXPECT_EQ(
		    ei, grenade::vx::common::ExecutionInstanceID(
		            grenade::vx::common::ExecutionIndex(3), DLSGlobal()));
		EXPECT_EQ(manager.get_current_hemisphere(), HemisphereOnDLS(0));
	}
}
