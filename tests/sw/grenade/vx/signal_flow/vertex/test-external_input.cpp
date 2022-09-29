#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/vertex/external_input.h"

using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex;

TEST(ExternalInput, General)
{
	EXPECT_NO_THROW(ExternalInput(ConnectionType::DataInt8, 123));
	EXPECT_THROW(ExternalInput(ConnectionType::Int8, 123), std::runtime_error);

	ExternalInput vertex(ConnectionType::DataInt8, 123);
	EXPECT_EQ(vertex.inputs().size(), 0);
	EXPECT_EQ(vertex.output().size, 123);

	EXPECT_EQ(vertex.output().type, ConnectionType::DataInt8);
}
