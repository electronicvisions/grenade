#pragma once

#include "grenade/vx/network/abstract/multicompartment/hardware_resource.h"

namespace grenade::vx::network::abstract {
struct HardwareResourceLeak;
} // namespace grenade::vx::network::abstract

namespace dapr {

extern template struct SYMBOL_VISIBLE EmptyProperty<
    grenade::vx::network::abstract::HardwareResourceLeak,
    grenade::vx::network::abstract::HardwareResource>;

} // namespace dapr

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceLeak
    : public dapr::EmptyProperty<HardwareResourceLeak, HardwareResource>
{
	HardwareResourceLeak() = default;
};

} // namespace grenade::vx::network::abstract