#pragma once

#include "grenade/vx/network/abstract/multicompartment/hardware_resource.h"

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceCapacity
    : public dapr::EmptyProperty<HardwareResourceCapacity, HardwareResource>
{
	HardwareResourceCapacity() = default;
};

} // namespace grenade::vx::network::abstract