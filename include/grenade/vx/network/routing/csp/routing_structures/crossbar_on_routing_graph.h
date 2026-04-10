#pragma once
#include "grenade/common/genpybind.h"
#include "halco/common/geometry.h"
#include "halco/common/geometry_numeric_limits.h"

namespace grenade::vx::network::routing::csp {


/**
 * Identifier of crossbar on routing graph.
 */
struct CrossbarOnRoutingGraph
    : public halco::common::detail::BaseType<CrossbarOnRoutingGraph, size_t>
{
	constexpr CrossbarOnRoutingGraph() = default;
	constexpr CrossbarOnRoutingGraph(value_type const& value) : base_t(value) {}
};


} // namespace grenade::vx::network::routing::csp


namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::routing::csp ::CrossbarOnRoutingGraph)
HALCO_GEOMETRY_NUMERIC_LIMITS_CLASS(grenade::vx::network::routing::csp ::CrossbarOnRoutingGraph)

} // namespace std