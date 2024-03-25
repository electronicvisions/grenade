#pragma once

#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"

namespace grenade::vx::network {

struct HardwareResourceCapacity;
extern template struct SYMBOL_VISIBLE EmptyProperty<HardwareResourceCapacity, HardwareResource>;

struct SYMBOL_VISIBLE HardwareResourceCapacity
    : public EmptyProperty<HardwareResourceCapacity, HardwareResource>
{
	HardwareResourceCapacity() = default;
};

} // namespace grenade::vx::network