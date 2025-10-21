#pragma once

#include "dapr/empty_property.h"
#include "dapr/property.h"
#include "dapr/property_holder.h"

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResource : public dapr::Property<HardwareResource>
{
	HardwareResource() = default;
};

} // namespace grenade::network::abstract