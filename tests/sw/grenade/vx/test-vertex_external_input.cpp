#include <gtest/gtest.h>

#include "grenade/vx/vertex/external_input.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(ExternalInput, General)
{
	EXPECT_NO_THROW(ExternalInput(ConnectionType::DataOutputUInt5, 123));

	ExternalInput vertex(ConnectionType::DataOutputUInt5, 123);
	EXPECT_EQ(vertex.inputs().size(), 0);
	EXPECT_EQ(vertex.output().size, 123);

	EXPECT_EQ(vertex.output().type, ConnectionType::DataOutputUInt5);
}
