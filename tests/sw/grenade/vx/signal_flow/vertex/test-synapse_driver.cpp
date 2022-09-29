#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/vertex/synapse_driver.h"

using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex;

TEST(SynapseDriver, General)
{
	SynapseDriver::Config config(
	    SynapseDriver::Config::RowAddressCompareMask(0b11000),
	    SynapseDriver::Config::RowModes{
	        SynapseDriver::Config::RowModes::value_type::excitatory,
	        SynapseDriver::Config::RowModes::value_type::inhibitory},
	    true);
	SynapseDriver::Coordinate coord(halco::common::Enum(5));

	SynapseDriver syndrv(coord, config);

	EXPECT_EQ(syndrv.inputs().size(), 1);
	EXPECT_EQ(syndrv.inputs().front().size, 1);
	EXPECT_EQ(syndrv.output().size, 1);

	EXPECT_EQ(syndrv.inputs().front().type, ConnectionType::SynapseDriverInputLabel);
	EXPECT_EQ(syndrv.output().type, ConnectionType::SynapseInputLabel);

	EXPECT_EQ(syndrv.get_coordinate(), coord);
	EXPECT_EQ(syndrv.get_config(), config);
}
