#pragma once

#include "grenade/common/vertex_on_topology.h"
#include "hate/visibility.h"

namespace grenade::vx::network::routing::csp {

/**
 * Source target pair.
 */
struct SYMBOL_VISIBLE SourceTargetPair
{
	grenade::common::VertexOnTopology source;
	grenade::common::VertexOnTopology target;

	SourceTargetPair(
	    grenade::common::VertexOnTopology source, grenade::common::VertexOnTopology target);

	auto operator<=>(const SourceTargetPair&) const = default;
};

std::ostream& operator<<(std::ostream& os, SourceTargetPair const& value);

} // namespace grenade::vx::network::routing::csp