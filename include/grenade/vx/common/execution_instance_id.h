#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/common/geometry.h"
#include "halco/common/mixin.h"
#include <cstddef>

namespace grenade::vx::common GENPYBIND_TAG_GRENADE_VX_COMMON {

/**
 * Execution instance identifier.
 * An execution instance describes a unique physically placed isolated execution.
 */
struct GENPYBIND(inline_base("*")) ExecutionInstanceID
    : public halco::common::detail::BaseType<ExecutionInstanceID, size_t>
{
	constexpr explicit ExecutionInstanceID(value_type const value = 0) : base_t(value) {}
};


HALCO_COORDINATE_MIXIN(ExecutionInstanceIDMixin, ExecutionInstanceID, execution_instance_id)

} // namespace grenade::vx::common

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::common::ExecutionInstanceID)

} // namespace std
