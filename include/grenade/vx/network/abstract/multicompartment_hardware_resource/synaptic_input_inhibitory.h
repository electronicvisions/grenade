#pragma once

#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"

namespace grenade::vx::network::abstract {
struct HardwareResourceSynapticInputInhibitory;
} // namespace grenade::vx::network::abstract

namespace dapr {
extern template struct SYMBOL_VISIBLE EmptyProperty<
    grenade::vx::network::abstract::HardwareResourceSynapticInputInhibitory,
    grenade::vx::network::abstract::HardwareResource>;
} // namespace dapr

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceSynapticInputInhibitory
    : public dapr::EmptyProperty<HardwareResourceSynapticInputInhibitory, HardwareResource>
{
	HardwareResourceSynapticInputInhibitory() = default;
};

} // namespace grenade::vx::network::abstract