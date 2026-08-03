#pragma once

#include "dapr/empty_hashable_property.h"
#include "dapr/property.h"
#include "dapr/property_holder.h"

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareResource
    : public dapr::Property<HardwareResource>
    , public dapr::Hashable
{
	HardwareResource() = default;
};

} // namespace grenade::network::abstract
