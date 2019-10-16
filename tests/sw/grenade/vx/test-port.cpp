#include <gtest/gtest.h>

#include "grenade/vx/port.h"

using namespace grenade::vx;

TEST(Port, General)
{
	Port port(123, ConnectionType::SynapseInputLabel);

	EXPECT_EQ(port.size, 123);
	EXPECT_EQ(port.type, ConnectionType::SynapseInputLabel);

	std::stringstream ss;
	ss << port;
	EXPECT_EQ(ss.str(), "Port(123, SynapseInputLabel)");

	Port port_other = port;
	port_other.size = 321;
	EXPECT_NE(port, port_other);
	port_other.size = port.size;
	port_other.type = ConnectionType::Int8;
	EXPECT_NE(port, port_other);

	Port port_copy = port;
	EXPECT_EQ(port, port_copy);
}
