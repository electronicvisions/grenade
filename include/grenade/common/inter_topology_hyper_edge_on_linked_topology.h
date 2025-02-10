#pragma once
#include "grenade/common/genpybind.h"
#include "halco/common/geometry.h"
#include "halco/common/geometry_numeric_limits.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Identifier of link on linked topology.
 */
struct GENPYBIND(inline_base("*")) InterTopologyHyperEdgeOnLinkedTopology
    : public halco::common::detail::BaseType<InterTopologyHyperEdgeOnLinkedTopology, size_t>
{
	constexpr InterTopologyHyperEdgeOnLinkedTopology() = default;
	constexpr InterTopologyHyperEdgeOnLinkedTopology(value_type const& value) : base_t(value) {}
};

} // namespace common
} // namespace grenade


namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::common::InterTopologyHyperEdgeOnLinkedTopology)
HALCO_GEOMETRY_NUMERIC_LIMITS_CLASS(grenade::common::InterTopologyHyperEdgeOnLinkedTopology)

} // namespace std
