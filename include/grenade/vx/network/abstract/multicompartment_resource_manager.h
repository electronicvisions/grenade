#pragma once

#include "grenade/common/property_holder.h"
#include "grenade/vx/network/abstract/multicompartment_compartment_on_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_environment.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_resource/capacitance.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_resource/synaptic_input_excitatory.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_resource/synaptic_input_inhibitory.h"
#include "grenade/vx/network/abstract/multicompartment_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_top_bottom.h"
#include <map>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

// Structure to know wich Compartment needs how many Neuron-Circuits and if TopBottom is required
struct GENPYBIND(visible) SYMBOL_VISIBLE ResourceManager
{
	CompartmentOnNeuron add_config(
	    CompartmentOnNeuron const& compartment,
	    Neuron const& neuron,
	    Environment const& environment);
	void remove_config(CompartmentOnNeuron const& compartment);
	NumberTopBottom const& get_config(CompartmentOnNeuron const& compartment) const;

private:
	std::map<CompartmentOnNeuron, common::PropertyHolder<NumberTopBottom>> resource_map;
};


} // namespace abstract
} // namespace grenade::vx::network
