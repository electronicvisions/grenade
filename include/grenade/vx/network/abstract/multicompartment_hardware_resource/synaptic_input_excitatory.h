#pragma once

#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"

namespace grenade::vx::network {
struct HardwareResourceSynapticInputExitatory;
} // namespace grenade::vx::network

namespace grenade::common {
extern template struct SYMBOL_VISIBLE EmptyProperty<
    vx::network::HardwareResourceSynapticInputExitatory,
    vx::network::HardwareResource>;
} // namespace grenade::common

namespace grenade::vx::network {

struct SYMBOL_VISIBLE HardwareResourceSynapticInputExitatory
    : public common::EmptyProperty<HardwareResourceSynapticInputExitatory, HardwareResource>
{
	HardwareResourceSynapticInputExitatory() = default;
};


} // namespace grenade::vx::network