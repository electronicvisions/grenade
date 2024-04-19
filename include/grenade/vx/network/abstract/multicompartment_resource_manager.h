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
	/**
	 * Adds the configuration for a compartment to the resource manager. Calculates the number of
	 * required resources of the compartment based on its mechanisms.
	 * @param compartment Compartment for which a configuration is added.
	 * @param neuron Neuron to which the compartment belongs.
	 * @param environment Environment specifying the synaptic inputs on the compartment.
	 */
	CompartmentOnNeuron add_config(
	    CompartmentOnNeuron const& compartment,
	    Neuron const& neuron,
	    Environment const& environment);

	/**
	 * Removes the configuration of a compartment from the resource manager.
	 * @param compartment Compartment for which the configuration is removed.
	 */
	void remove_config(CompartmentOnNeuron const& compartment);

	/**
	 * Return the configuration for a compartment in the resource maanger.
	 * @param compartment Compartment whichs configuration is returned.
	 */
	NumberTopBottom const& get_config(CompartmentOnNeuron const& compartment) const;

	/**
	 * Return the total resource requirements of all compartments inside the resource manager.
	 */
	NumberTopBottom const& get_total() const;

	/**
	 * Adds the configuration for all compartments of a neuron.
	 * @param neuron Neuron for which configuration is added.
	 * @param environment Environment which specifies the synaptic inputs on the compartments of the
	 * neuron.
	 */
	void add_config(Neuron const& neuron, Environment const& environment);

private:
	std::map<CompartmentOnNeuron, common::PropertyHolder<NumberTopBottom>> resource_map;
	NumberTopBottom m_total;
};


} // namespace abstract
} // namespace grenade::vx::network
