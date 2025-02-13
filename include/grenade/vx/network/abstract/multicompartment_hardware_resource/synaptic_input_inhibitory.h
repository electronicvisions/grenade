#pragma once

#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"

namespace grenade::vx::network {
struct HardwareResourceSynapticInputInhibitory;
} // namespace grenade::vx::network

namespace grenade::common {
extern template struct SYMBOL_VISIBLE common::EmptyProperty<
    vx::network::HardwareResourceSynapticInputInhibitory,
    vx::network::HardwareResource>;
} // namespace grenade::common

namespace grenade::vx::network {

struct SYMBOL_VISIBLE HardwareResourceSynapticInputInhibitory
    : public common::EmptyProperty<HardwareResourceSynapticInputInhibitory, HardwareResource>
{
	HardwareResourceSynapticInputInhibitory() = default;
};

} // namespace grenade::vx::network