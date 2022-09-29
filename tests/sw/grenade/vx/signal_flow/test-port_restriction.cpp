#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/port_restriction.h"

#include "grenade/vx/signal_flow/port.h"

using namespace grenade::vx::signal_flow;

TEST(PortRestriction, General)
{
	EXPECT_THROW((PortRestriction(10, 5)), std::runtime_error);
	EXPECT_THROW((PortRestriction{10, 5}), std::runtime_error);
	EXPECT_NO_THROW((PortRestriction(5, 10)));
	EXPECT_NO_THROW((PortRestriction(10, 10)));

	{
		PortRestriction port_restriction{5, 10};
		EXPECT_EQ(port_restriction.min(), 5);
		EXPECT_EQ(port_restriction.max(), 10);
		EXPECT_EQ(port_restriction.size(), 6);
	}

	PortRestriction port_restriction(5, 10);
	EXPECT_EQ(port_restriction.min(), 5);
	EXPECT_EQ(port_restriction.max(), 10);
	EXPECT_EQ(port_restriction.size(), 6);

	std::stringstream ss;
	ss << port_restriction;
	EXPECT_EQ(ss.str(), "PortRestriction(min: 5, max: 10)");

	{
		Port port(123, ConnectionType::Int8);
		EXPECT_TRUE(port_restriction.is_restriction_of(port));
	}

	{
		Port port(7, ConnectionType::Int8);
		EXPECT_FALSE(port_restriction.is_restriction_of(port));
	}
}
