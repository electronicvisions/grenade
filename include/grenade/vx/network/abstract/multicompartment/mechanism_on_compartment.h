#pragma once

#include "halco/common/geometry.h"
#include "halco/common/geometry_numeric_limits.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

// Mechanism-ID
struct GENPYBIND(inline_base("*")) SYMBOL_VISIBLE MechanismOnCompartment
    : public halco::common::detail::BaseType<MechanismOnCompartment, size_t>
{
	constexpr MechanismOnCompartment(value_type const value = 0) : base_t(value) {}
};

} // namespace abstract
} // namespace grenade::vx::network

namespace std {

HALCO_GEOMETRY_NUMERIC_LIMITS_CLASS(grenade::vx::network::abstract::MechanismOnCompartment)

} // namespace std
