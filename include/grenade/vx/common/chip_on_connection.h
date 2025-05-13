#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/common/geometry.h"
#include <cstddef>

namespace grenade::vx {
namespace common GENPYBIND_TAG_GRENADE_VX_COMMON {

/**
 * Identifier of a chip on a connection.
 */
struct GENPYBIND(inline_base("*")) ChipOnConnection
    : public halco::common::detail::BaseType<ChipOnConnection, size_t>
{
	constexpr explicit ChipOnConnection(value_type const value = 0) : base_t(value) {}
};

} // namespace common
} // namespace grenade::vx

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::common::ChipOnConnection)

} // namespace std
