#pragma once

#include "grenade/vx/network/abstract/multicompartment/hardware_resource.h"

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceSynapticInputInhibitory
    : public dapr::EmptyProperty<HardwareResourceSynapticInputInhibitory, HardwareResource>
{
	HardwareResourceSynapticInputInhibitory() = default;
};

} // namespace grenade::vx::network::abstract