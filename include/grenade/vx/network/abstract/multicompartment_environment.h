#pragma once

#include "grenade/common/detail/property_holder.h"
#include "grenade/vx/network/abstract/multicompartment_compartment_on_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_synaptic_input_environment.h"
#include <map>

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) SYMBOL_VISIBLE Environment
{
	std::map<
	    CompartmentOnNeuron,
	    std::vector<common::detail::PropertyHolder<SynapticInputEnvironment>>>
	    synaptic_connections;
};

} // namespace network
} // namespace grenade::vx
