#pragma once

#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"

namespace grenade::vx::network {
struct HardwareResourceCapacity;
} // namespace grenade::vx::network

namespace grenade::common {

extern template struct SYMBOL_VISIBLE
    common::EmptyProperty<vx::network::HardwareResourceCapacity, vx::network::HardwareResource>;

} // namespace grenade::common

namespace grenade::vx::network {

struct SYMBOL_VISIBLE HardwareResourceCapacity
    : public common::EmptyProperty<HardwareResourceCapacity, HardwareResource>
{
	HardwareResourceCapacity() = default;
};

} // namespace grenade::vx::network