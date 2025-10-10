#pragma once
#include "grenade/common/genpybind.h"
#include "halco/common/geometry.h"
#include "halco/common/geometry_numeric_limits.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Identifier of vertex on topology.
 */
struct GENPYBIND(inline_base("*")) VertexOnTopology
    : public halco::common::detail::BaseType<VertexOnTopology, size_t>
{
	constexpr VertexOnTopology() = default;
	constexpr VertexOnTopology(value_type const& value) : base_t(value) {}
};

} // namespace common
} // namespace grenade


namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::common::VertexOnTopology)
HALCO_GEOMETRY_NUMERIC_LIMITS_CLASS(grenade::common::VertexOnTopology)

} // namespace std
