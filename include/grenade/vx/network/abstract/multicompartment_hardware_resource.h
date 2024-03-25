#pragma once

#include "grenade/vx/network/abstract/detail/property_holder.h"
#include "grenade/vx/network/abstract/empty_property.h"
#include "grenade/vx/network/abstract/property.h"

namespace grenade::vx::network {

struct SYMBOL_VISIBLE HardwareResource : public Property<HardwareResource>
{
	HardwareResource() = default;
};

} // namespace grenade::network::abstract