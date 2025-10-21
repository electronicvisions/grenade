#pragma once

#include "dapr/property_holder.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_constraint.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"
#include <vector>

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourcesWithConstraints
{
	std::vector<dapr::PropertyHolder<HardwareResource>> resources;
	std::vector<dapr::PropertyHolder<HardwareConstraint>> constraints;
};


} // namespace grenade::vx::network::abstract