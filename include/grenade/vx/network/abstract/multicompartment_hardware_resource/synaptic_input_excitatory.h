#pragma once

#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"

namespace grenade::vx::network::abstract {
struct HardwareResourceSynapticInputExitatory;
} // namespace grenade::vx::network::abstract

namespace grenade::common {
extern template struct SYMBOL_VISIBLE EmptyProperty<
    vx::network::abstract::HardwareResourceSynapticInputExitatory,
    vx::network::abstract::HardwareResource>;
} // namespace grenade::common

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceSynapticInputExitatory
    : public common::EmptyProperty<HardwareResourceSynapticInputExitatory, HardwareResource>
{
	HardwareResourceSynapticInputExitatory() = default;
};


} // namespace grenade::vx::network::abstract