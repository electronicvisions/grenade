#pragma once

#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"

namespace grenade::vx::network::abstract {
struct HardwareResourceCapacity;
} // namespace grenade::vx::network::abstract

namespace dapr {

extern template struct SYMBOL_VISIBLE EmptyProperty<
    grenade::vx::network::abstract::HardwareResourceCapacity,
    grenade::vx::network::abstract::HardwareResource>;

} // namespace dapr

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceCapacity
    : public dapr::EmptyProperty<HardwareResourceCapacity, HardwareResource>
{
	HardwareResourceCapacity() = default;
};

} // namespace grenade::vx::network::abstract