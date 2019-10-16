#include <gtest/gtest.h>

#include "grenade/vx/vertex/cadc_readout.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(CADCMembraneReadoutView, General)
{
	EXPECT_NO_THROW(CADCMembraneReadoutView(256));
	EXPECT_THROW(CADCMembraneReadoutView(257), std::out_of_range);

	CADCMembraneReadoutView vertex(123);
	EXPECT_EQ(vertex.inputs().size(), 1);
	EXPECT_EQ(vertex.inputs().front().size, 123);
	EXPECT_EQ(vertex.output().size, 123);

	EXPECT_EQ(vertex.inputs().front().type, ConnectionType::MembraneVoltage);
	EXPECT_EQ(vertex.output().type, ConnectionType::Int8);
}
