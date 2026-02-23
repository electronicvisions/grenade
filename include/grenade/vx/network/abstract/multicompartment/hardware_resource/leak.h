#pragma once

#include "grenade/vx/network/abstract/multicompartment/hardware_resource.h"

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceLeak
    : public dapr::EmptyProperty<HardwareResourceLeak, HardwareResource>
{
	HardwareResourceLeak() = default;
};

} // namespace grenade::vx::network::abstract