#pragma once

#include "dapr/property_holder.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_constraint.h"
#include <vector>

namespace grenade::vx::network::abstract {

typedef std::vector<dapr::PropertyHolder<HardwareConstraint>> HardwareConstraints;

} // namespace grenade::vx::network::abstract