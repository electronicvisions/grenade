#pragma once
#include "grenade/common/genpybind.h"
#include "halco/common/geometry.h"
#include "halco/common/geometry_numeric_limits.h"


namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Identifier of edge on topology.
 */
struct GENPYBIND(inline_base("*")) EdgeOnTopology
    : public halco::common::detail::BaseType<EdgeOnTopology, size_t>
{
	constexpr EdgeOnTopology() = default;
	constexpr EdgeOnTopology(value_type const& value) : base_t(value) {}
};

} // namespace common
} // namespace grenade


namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::common::EdgeOnTopology)
HALCO_GEOMETRY_NUMERIC_LIMITS_CLASS(grenade::common::EdgeOnTopology)

} // namespace std
