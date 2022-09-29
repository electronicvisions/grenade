#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/input.h"

using namespace grenade::vx::signal_flow;

TEST(Input, General)
{
	{
		Input input(5);
		EXPECT_EQ(input.descriptor, 5);
		EXPECT_EQ(input.port_restriction, std::nullopt);

		std::stringstream ss;
		ss << input;
		EXPECT_EQ(ss.str(), "Input(descriptor: 5)");

		Input eq_input = {5};
		EXPECT_EQ(input, eq_input);
		Input ne_input = {6};
		EXPECT_NE(input, ne_input);
	}

	{
		Input input(5, PortRestriction(5, 10));
		EXPECT_EQ(input.descriptor, 5);
		EXPECT_TRUE(input.port_restriction);
		EXPECT_EQ(*input.port_restriction, PortRestriction(5, 10));

		std::stringstream ss;
		ss << input;
		EXPECT_EQ(ss.str(), "Input(descriptor: 5, PortRestriction(min: 5, max: 10))");

		Input eq_input = {5, {5, 10}};
		EXPECT_EQ(input, eq_input);
		Input ne_input = {5, {5, 6}};
		EXPECT_NE(input, ne_input);
	}
}
