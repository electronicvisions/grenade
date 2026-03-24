#pragma once

#include "grenade/vx/network/abstract/multicompartment/hardware_resource.h"

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceFire
    : public dapr::EmptyHashableProperty<HardwareResourceFire, HardwareResource>
{
	HardwareResourceFire() = default;
};

} // namespace grenade::vx::network::abstract
