#include <gtest/gtest.h>

#include "grenade/vx/vertex/data_output.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(DataOutput, General)
{
	EXPECT_NO_THROW(DataOutput(ConnectionType::SynapseInputLabel, 123));
	EXPECT_THROW(DataOutput(ConnectionType::SynapticInput, 123), std::runtime_error);

	DataOutput vertex(ConnectionType::SynapseInputLabel, 123);
	EXPECT_EQ(vertex.inputs().size(), 1);
	EXPECT_EQ(vertex.inputs().front().size, 123);
	EXPECT_EQ(vertex.output().size, 123);

	EXPECT_EQ(vertex.inputs().front().type, ConnectionType::SynapseInputLabel);
	EXPECT_EQ(vertex.output().type, ConnectionType::DataOutputUInt5);
}
