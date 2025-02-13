#pragma once

#include "grenade/common/property_holder.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_constraint.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"
#include <vector>

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourcesWithConstraints
{
	std::vector<common::PropertyHolder<HardwareResource>> resources;
	std::vector<common::PropertyHolder<HardwareConstraint>> constraints;
};


} // namespace grenade::vx::network::abstract