#include <gtest/gtest.h>

#include "grenade/vx/network/placed_logical/build_connection_weight_split.h"
#include "grenade/vx/network/placed_logical/projection.h"

using SynapseWeight = lola::vx::v3::SynapseMatrix::Weight;
using namespace grenade::vx::network::placed_logical;

TEST(build_connection_weight_split, General)
{
	{
		// no split required, no split requested
		Projection::Connection::Weight weight(63);
		auto const split = build_connection_weight_split(weight, 1);
		EXPECT_EQ(split, std::vector{SynapseWeight(63)});
	}
	{
		// no split required, split requested
		Projection::Connection::Weight weight(63);
		auto const split = build_connection_weight_split(weight, 2);
		EXPECT_EQ(split, (std::vector{SynapseWeight(63), SynapseWeight(0)}));
	}
	{
		// split required, exact split requested
		Projection::Connection::Weight weight(64);
		auto const split = build_connection_weight_split(weight, 2);
		EXPECT_EQ(split, (std::vector{SynapseWeight(63), SynapseWeight(1)}));
	}
	{
		// split required, split requested
		Projection::Connection::Weight weight(64);
		auto const split = build_connection_weight_split(weight, 3);
		EXPECT_EQ(split, (std::vector{SynapseWeight(63), SynapseWeight(1), SynapseWeight(0)}));
	}
	{
		// split required, split requested
		Projection::Connection::Weight weight(140);
		auto const split = build_connection_weight_split(weight, 4);
		EXPECT_EQ(
		    split, (std::vector{
		               SynapseWeight(63), SynapseWeight(63), SynapseWeight(14), SynapseWeight(0)}));
	}
	{
		// requested split too small
		Projection::Connection::Weight weight(140);
		EXPECT_THROW((build_connection_weight_split(weight, 2)), std::overflow_error);
	}
}
