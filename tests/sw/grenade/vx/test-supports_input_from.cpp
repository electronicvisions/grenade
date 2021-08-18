#include <gtest/gtest.h>

#include "grenade/vx/port.h"
#include "grenade/vx/port_restriction.h"
#include "grenade/vx/supports_input_from.h"
#include <optional>

using namespace grenade::vx;

/**
 * Dummy vertex without supports_input_from method.
 */
struct VertexA
{};

/**
 * Dummy vertex with supports_input_from method.
 */
struct VertexB
{
	std::optional<PortRestriction> restriction_reference;

	bool supports_input_from(
	    VertexA const&, std::optional<PortRestriction> const& restriction) const
	{
		return restriction == restriction_reference;
	}
};

TEST(supports_input_from, General)
{
	VertexA vertex_a;
	// no member supports_input_from leads to supporting everything
	EXPECT_TRUE(supports_input_from(vertex_a, vertex_a, std::nullopt));
	EXPECT_TRUE(supports_input_from(vertex_a, vertex_a, PortRestriction(12, 13)));

	VertexB vertex_b{std::nullopt};
	// checking that correct restriction is forwarded
	EXPECT_TRUE(supports_input_from(vertex_b, vertex_a, std::nullopt));
	EXPECT_FALSE(supports_input_from(vertex_b, vertex_a, PortRestriction(12, 13)));
	vertex_b.restriction_reference = PortRestriction(12, 13);
	EXPECT_FALSE(supports_input_from(vertex_b, vertex_a, std::nullopt));
	EXPECT_TRUE(supports_input_from(vertex_b, vertex_a, PortRestriction(12, 13)));
	// any member but no matching member leads to support
	EXPECT_TRUE(supports_input_from(vertex_b, vertex_b, std::nullopt));
	EXPECT_TRUE(supports_input_from(vertex_b, vertex_b, PortRestriction(12, 13)));
}
