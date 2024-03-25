#pragma once

#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"

namespace grenade::vx::network {

struct HardwareResourceSynapticInputExitatory;
extern template struct SYMBOL_VISIBLE
    EmptyProperty<HardwareResourceSynapticInputExitatory, HardwareResource>;

struct SYMBOL_VISIBLE HardwareResourceSynapticInputExitatory
    : public EmptyProperty<HardwareResourceSynapticInputExitatory, HardwareResource>
{
	HardwareResourceSynapticInputExitatory() = default;
};


} // namespace grenade::vx::network