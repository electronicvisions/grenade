#pragma once

#include "grenade/common/detail/property_holder.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_constraint.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"
#include <vector>

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourcesWithConstraints
{
	std::vector<common::detail::PropertyHolder<HardwareResource>> resources;
	std::vector<common::detail::PropertyHolder<HardwareConstraint>> constraints;
};


} // namespace grenade::vx::network::abstract