#pragma once
#include "grenade/common/genpybind.h"
#include "halco/common/geometry.h"
#include <cstddef>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Identifier of connection to a backend on an executor.
 * Executions can only be synchronous intra-connection.
 */
struct GENPYBIND(inline_base("*")) ConnectionOnExecutor
    : public halco::common::detail::BaseType<ConnectionOnExecutor, size_t>
{
	constexpr explicit ConnectionOnExecutor(value_type const value = 0) : base_t(value) {}
};

} // namespace common
} // namespace grenade

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::common::ConnectionOnExecutor)

} // namespace std
