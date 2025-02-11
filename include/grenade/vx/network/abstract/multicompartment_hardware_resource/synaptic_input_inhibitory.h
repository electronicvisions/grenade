#pragma once

#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"

namespace grenade::vx::network::abstract {
struct HardwareResourceSynapticInputInhibitory;
} // namespace grenade::vx::network::abstract

namespace grenade::common {
extern template struct SYMBOL_VISIBLE common::EmptyProperty<
    vx::network::abstract::HardwareResourceSynapticInputInhibitory,
    vx::network::abstract::HardwareResource>;
} // namespace grenade::common

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResourceSynapticInputInhibitory
    : public common::EmptyProperty<HardwareResourceSynapticInputInhibitory, HardwareResource>
{
	HardwareResourceSynapticInputInhibitory() = default;
};

} // namespace grenade::vx::network::abstract