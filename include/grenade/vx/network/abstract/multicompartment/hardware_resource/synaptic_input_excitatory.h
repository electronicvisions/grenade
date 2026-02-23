#pragma once

#include "grenade/vx/network/abstract/multicompartment/hardware_resource.h"

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceSynapticInputExitatory
    : public dapr::EmptyProperty<HardwareResourceSynapticInputExitatory, HardwareResource>
{
	HardwareResourceSynapticInputExitatory() = default;
};


} // namespace grenade::vx::network::abstract