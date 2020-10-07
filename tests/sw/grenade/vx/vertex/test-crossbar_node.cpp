#include <gtest/gtest.h>

#include "grenade/vx/vertex/crossbar_node.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(CrossbarNode, General)
{
	CrossbarNode::Coordinate coord(halco::common::Enum(5));
	CrossbarNode::Config cfg;
	cfg.set_enable_drop_counter(!cfg.get_enable_drop_counter());
	CrossbarNode config(coord, cfg);

	EXPECT_EQ(config.inputs().size(), 1);
	EXPECT_EQ(config.inputs().front().size, 1);
	EXPECT_EQ(config.output().size, 1);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::CrossbarInputLabel);
	EXPECT_EQ(config.output().type, ConnectionType::CrossbarOutputLabel);

	EXPECT_EQ(config.get_coordinate(), coord);
	EXPECT_EQ(config.get_config(), cfg);
}
