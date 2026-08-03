#pragma once

#include "grenade/vx/network/abstract/multicompartment/hardware_resource.h"

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceAnalogReadout
    : public dapr::EmptyHashableProperty<HardwareResourceAnalogReadout, HardwareResource>
{
	HardwareResourceAnalogReadout() = default;
};

} // namespace grenade::vx::network::abstract
