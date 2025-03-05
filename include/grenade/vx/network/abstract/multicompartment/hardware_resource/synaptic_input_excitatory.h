#pragma once

#include "grenade/vx/network/abstract/multicompartment/hardware_resource.h"

namespace grenade::vx::network::abstract {
struct HardwareResourceSynapticInputExitatory;
} // namespace grenade::vx::network::abstract

namespace dapr {
extern template struct SYMBOL_VISIBLE EmptyProperty<
    grenade::vx::network::abstract::HardwareResourceSynapticInputExitatory,
    grenade::vx::network::abstract::HardwareResource>;
} // namespace dapr

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceSynapticInputExitatory
    : public dapr::EmptyProperty<HardwareResourceSynapticInputExitatory, HardwareResource>
{
	HardwareResourceSynapticInputExitatory() = default;
};


} // namespace grenade::vx::network::abstract