#pragma once

#include "grenade/common/empty_property.h"
#include "grenade/common/property.h"
#include "grenade/common/property_holder.h"

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResource : public common::Property<HardwareResource>
{
	HardwareResource() = default;
};

} // namespace grenade::network::abstract