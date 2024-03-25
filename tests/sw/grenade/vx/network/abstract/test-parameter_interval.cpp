#include "grenade/vx/network/abstract/parameter_interval.h"
#include <gtest/gtest.h>

using namespace grenade::vx::network;

TEST(multicompartment_neuron, parameter_interval)
{
	ParameterInterval<int> parameter_interval_a;

	ParameterInterval<int> parameter_interval_b(2, 4);

	EXPECT_EQ(parameter_interval_b.get_lower(), 2);

	EXPECT_NE(parameter_interval_a, parameter_interval_b);

	parameter_interval_a = ParameterInterval<int>(2, 4);
	EXPECT_EQ(parameter_interval_a, parameter_interval_b);

	EXPECT_THROW(ParameterInterval<int>(4, 2), std::invalid_argument);
}