#pragma once

#include "dapr/property.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_constraint.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct Environment;
struct CompartmentOnNeuron;

// Mechanism Base-Class
struct GENPYBIND(inline_base("*")) SYMBOL_VISIBLE Mechanism : public dapr::Property<Mechanism>
{
	virtual bool conflict(Mechanism const& other) const = 0;
	virtual HardwareResourcesWithConstraints get_hardware(
	    CompartmentOnNeuron const& compartment, Environment const& environment) const = 0;
	virtual bool valid() const = 0;
};


} // namespace abstract
} // namespace grenade::vx::network
