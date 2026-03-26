#pragma once

#include "grenade/vx/network/abstract/multicompartment/hardware_resource.h"

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceEventOutput
    : public dapr::EmptyHashableProperty<HardwareResourceEventOutput, HardwareResource>
{
	HardwareResourceEventOutput() = default;
};

} // namespace grenade::vx::network::abstract
