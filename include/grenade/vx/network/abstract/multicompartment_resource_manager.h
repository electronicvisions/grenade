#pragma once

#include "dapr/property_holder.h"
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
	 * Returns all compartments for which resource requirements are stored.
	 */
	std::vector<CompartmentOnNeuron> get_compartments() const;

	/**
	 * Adds the configuration for all compartments of a neuron.
	 * @param neuron Neuron for which configuration is added.
	 * @param environment Environment which specifies the synaptic inputs on the compartments of the
	 * neuron.
	 */
	void add_config(Neuron const& neuron, Environment const& environment);

	/**
	 * Writes neuron topology in graphviz format.
	 * Contains information about the resources required by each compartment of the neuron.
	 * @param filename File to write graph to.
	 * @param neuron Neuron to be written as graph.
	 * @param name Name of the neuron graph.
	 * @param append Whether to append the neuron to the file or erase all other contents.
	 */
	void write_graphviz(
	    std::string filename, Neuron const& neuron, std::string name, bool append = false);

	// Returns list of required MADC-recordable pairs.
	std::set<std::pair<CompartmentOnNeuron, CompartmentOnNeuron>> get_recordable_pairs() const;

private:
	std::set<std::pair<CompartmentOnNeuron, CompartmentOnNeuron>> m_recordable_pairs;

	std::map<CompartmentOnNeuron, dapr::PropertyHolder<NumberTopBottom>> resource_map;
	NumberTopBottom m_total;
};

std::ostream& operator<<(std::ostream& os, ResourceManager const& resources) SYMBOL_VISIBLE;

} // namespace abstract
} // namespace grenade::vx::network
