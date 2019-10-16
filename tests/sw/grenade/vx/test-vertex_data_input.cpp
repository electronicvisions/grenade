#include <gtest/gtest.h>

#include "grenade/vx/vertex/data_input.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(DataInput, General)
{
	EXPECT_NO_THROW(DataInput(ConnectionType::SynapseInputLabel, 123));
	EXPECT_THROW(DataInput(ConnectionType::SynapticInput, 123), std::runtime_error);

	DataInput vertex(ConnectionType::SynapseInputLabel, 123);
	EXPECT_EQ(vertex.inputs().size(), 1);
	EXPECT_EQ(vertex.inputs().front().size, 123);
	EXPECT_EQ(vertex.output().size, 123);

	EXPECT_EQ(vertex.inputs().front().type, ConnectionType::DataOutputUInt5);
	EXPECT_EQ(vertex.output().type, ConnectionType::SynapseInputLabel);
}
