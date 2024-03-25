#pragma once

#include "halco/common/geometry.h"

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

// Mechanism-ID
struct GENPYBIND(visible) SYMBOL_VISIBLE MechanismOnCompartment
    : public halco::common::detail::BaseType<MechanismOnCompartment, size_t>
{
	MechanismOnCompartment(value_type const value = 0) : base_t(value) {}
};

} // namespace grenade::vx::network