#pragma once

#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"

namespace grenade::vx::network::abstract {
struct HardwareResourceCapacity;
} // namespace grenade::vx::network::abstract

namespace grenade::common {

extern template struct SYMBOL_VISIBLE common::EmptyProperty<
    vx::network::abstract::HardwareResourceCapacity,
    vx::network::abstract::HardwareResource>;

} // namespace grenade::common

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceCapacity
    : public common::EmptyProperty<HardwareResourceCapacity, HardwareResource>
{
	HardwareResourceCapacity() = default;
};

} // namespace grenade::vx::network::abstract