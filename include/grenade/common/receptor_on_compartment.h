#pragma once
#include "grenade/common/genpybind.h"
#include "halco/common/geometry.h"
#include <memory>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Identifier of receptor on compartment.
 */
struct GENPYBIND(inline_base("*")) ReceptorOnCompartment
    : public halco::common::detail::BaseType<ReceptorOnCompartment, size_t>
{
	constexpr explicit ReceptorOnCompartment(value_type const value = 0) : base_t(value) {}
};

} // namespace common
} // namespace grenade
