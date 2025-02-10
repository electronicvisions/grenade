#pragma once
#include "grenade/common/genpybind.h"
#include "halco/common/geometry.h"
#include <memory>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Time domain on topology.
 * Describes realtime section on which the vertex is executed.
 * Vertices residing on a time domain can only be connected on the same time domain.
 */
struct GENPYBIND(inline_base("*")) TimeDomainOnTopology
    : public halco::common::detail::BaseType<TimeDomainOnTopology, size_t>
{
	constexpr explicit TimeDomainOnTopology(value_type const value = 0) : base_t(value) {}
};

} // namespace common
} // namespace grenade

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::common::TimeDomainOnTopology)

} // namespace std
