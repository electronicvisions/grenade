#pragma once
#include "dapr/property.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"


namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) MechanismEnvironment
    : public dapr::Property<MechanismEnvironment>
{};

} // abstract
} // namespace grenade::vx::network
