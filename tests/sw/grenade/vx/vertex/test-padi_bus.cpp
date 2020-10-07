#include <gtest/gtest.h>

#include "grenade/vx/vertex/padi_bus.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(PADIBus, General)
{
	PADIBus::Coordinate coord(halco::common::Enum(5));
	PADIBus config(coord);

	EXPECT_EQ(config.inputs().size(), 1);
	EXPECT_EQ(config.inputs().front().size, 1);
	EXPECT_EQ(config.output().size, 1);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::CrossbarOutputLabel);
	EXPECT_EQ(config.output().type, ConnectionType::SynapseDriverInputLabel);

	EXPECT_EQ(config.get_coordinate(), coord);
}
