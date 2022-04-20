#include <gtest/gtest.h>

#include "grenade/vx/vertex/crossbar_node.h"

#include "grenade/vx/vertex/background_spike_source.h"

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

	{
		BackgroundSpikeSource source({}, BackgroundSpikeSource::Coordinate(2));
		EXPECT_FALSE(config.supports_input_from(source, std::nullopt));
	}
	{
		CrossbarNode::Coordinate coord(
		    halco::hicann_dls::vx::v3::CrossbarOutputOnDLS(2),
		    halco::hicann_dls::vx::v3::CrossbarInputOnDLS(14));
		CrossbarNode::Config cfg;
		CrossbarNode config(coord, cfg);
		BackgroundSpikeSource source({}, BackgroundSpikeSource::Coordinate(2));
		EXPECT_TRUE(config.supports_input_from(source, std::nullopt));
	}
}
