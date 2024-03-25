#pragma once

#include "grenade/vx/network/abstract/detail/property_holder.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_constraint.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"
#include <vector>

namespace grenade::vx::network {

struct SYMBOL_VISIBLE HardwareResourcesWithConstraints
{
	std::vector<detail::PropertyHolder<HardwareResource>> resources;
	std::vector<detail::PropertyHolder<HardwareConstraint>> constraints;
};


} // namespace grenade::vx::network