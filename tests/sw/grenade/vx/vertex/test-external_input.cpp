#include <gtest/gtest.h>

#include "grenade/vx/vertex/external_input.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(ExternalInput, General)
{
	EXPECT_NO_THROW(ExternalInput(ConnectionType::DataInt8, 123));

	ExternalInput vertex(ConnectionType::DataInt8, 123);
	EXPECT_EQ(vertex.inputs().size(), 0);
	EXPECT_EQ(vertex.output().size, 123);

	EXPECT_EQ(vertex.output().type, ConnectionType::DataInt8);
}
