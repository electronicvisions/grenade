#pragma once

#include "grenade/common/property_holder.h"
#include "grenade/vx/network/abstract/multicompartment_compartment_on_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_synaptic_input_environment.h"
#include <map>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) SYMBOL_VISIBLE Environment
{
	std::map<CompartmentOnNeuron, std::vector<common::PropertyHolder<SynapticInputEnvironment>>>
	    synaptic_connections;
};

} // namespace abstract
} // namespace grenade::vx::network
