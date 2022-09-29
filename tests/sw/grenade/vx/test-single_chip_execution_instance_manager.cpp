#include <gtest/gtest.h>

#include "grenade/vx/single_chip_execution_instance_manager.h"

using namespace halco::hicann_dls::vx::v3;

TEST(SingleChipExecutionInstanceManager, General)
{
	grenade::vx::SingleChipExecutionInstanceManager manager;

	EXPECT_EQ(manager.get_current_hemisphere(), HemisphereOnDLS(0));

	{
		auto const ei = manager.next();
		EXPECT_EQ(ei, grenade::vx::signal_flow::ExecutionInstance());
		EXPECT_EQ(manager.get_current_hemisphere(), HemisphereOnDLS(1));
	}
	{
		auto const ei = manager.next();
		EXPECT_EQ(
		    ei, grenade::vx::signal_flow::ExecutionInstance(
		            grenade::vx::signal_flow::ExecutionIndex(1), DLSGlobal()));
		EXPECT_EQ(manager.get_current_hemisphere(), HemisphereOnDLS(0));
	}
	{
		auto const ei = manager.next();
		EXPECT_EQ(
		    ei, grenade::vx::signal_flow::ExecutionInstance(
		            grenade::vx::signal_flow::ExecutionIndex(1), DLSGlobal()));
		EXPECT_EQ(manager.get_current_hemisphere(), HemisphereOnDLS(1));
	}
	{
		auto const ei = manager.next_index();
		EXPECT_EQ(
		    ei, grenade::vx::signal_flow::ExecutionInstance(
		            grenade::vx::signal_flow::ExecutionIndex(2), DLSGlobal()));
		EXPECT_EQ(manager.get_current_hemisphere(), HemisphereOnDLS(0));
	}
	{
		auto const ei = manager.next_index();
		EXPECT_EQ(
		    ei, grenade::vx::signal_flow::ExecutionInstance(
		            grenade::vx::signal_flow::ExecutionIndex(3), DLSGlobal()));
		EXPECT_EQ(manager.get_current_hemisphere(), HemisphereOnDLS(0));
	}
}
