#pragma once

#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"

namespace grenade::vx::network {

struct HardwareResourceSynapticInputInhibitory;
extern template struct SYMBOL_VISIBLE
    EmptyProperty<HardwareResourceSynapticInputInhibitory, HardwareResource>;

struct SYMBOL_VISIBLE HardwareResourceSynapticInputInhibitory
    : public EmptyProperty<HardwareResourceSynapticInputInhibitory, HardwareResource>
{
	HardwareResourceSynapticInputInhibitory() = default;
};

} // namespace grenade::vx::network