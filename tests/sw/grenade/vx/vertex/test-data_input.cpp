#include <gtest/gtest.h>

#include "grenade/vx/vertex/data_input.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(DataInput, General)
{
	EXPECT_NO_THROW(DataInput(ConnectionType::Int8, 123));
	EXPECT_THROW(DataInput(ConnectionType::SynapticInput, 123), std::runtime_error);

	DataInput vertex(ConnectionType::Int8, 123);
	EXPECT_EQ(vertex.inputs().size(), 1);
	EXPECT_EQ(vertex.inputs().front().size, 123);
	EXPECT_EQ(vertex.output().size, 123);

	EXPECT_EQ(vertex.inputs().front().type, ConnectionType::DataOutputInt8);
	EXPECT_EQ(vertex.output().type, ConnectionType::Int8);
}
