#pragma once

#include "dapr/property_holder.h"
#include "grenade/vx/network/abstract/multicompartment/compartment_on_neuron.h"
#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/capacitance.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/synaptic_input_excitatory.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/synaptic_input_inhibitory.h"
#include "grenade/vx/network/abstract/multicompartment/neuron.h"
#include "grenade/vx/network/abstract/multicompartment/top_bottom.h"
#include <map>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

// Structure to know wich Compartment needs how many Neuron-Circuits and if TopBottom is required
struct GENPYBIND(visible) SYMBOL_VISIBLE ResourceManager
{
	/**
	 * Add the configuration for a neuron to the resource manager. Calculates the resources required
	 * for placement depending on the mechanisms, compartments and parameterization of the neuron.
	 *
	 * @param neuron Neuron for which a configuration is added.
	 * @param parameter_space Parameter-Space of the neuron.
	 * @param environment Environment defining synaptic inputs on neuron.
	 */
	void add_config(
	    Neuron const& neuron,
	    Neuron::ParameterSpace const& parameter_space,
	    Environment const& environment);

	/**
	 * Remove the configuration of the given neuron from the resource manager.
	 *
	 * @param neuron Neuron for which the configuration is removed.
	 */
	void remove_config(Neuron const& neuron);

	/**
	 * Return the configuration for a compartment in the resource maanger.
	 *
	 * @param compartment Compartment of which configuration is returned.
	 */
	NumberTopBottom const& get_config(CompartmentOnNeuron const& compartment) const;

	/**
	 * Returns total resources required by the contents of the resource manager.
	 */
	NumberTopBottom const& get_total() const;

	/**
	 * Returns descriptors for all compartments for which a configuration is stored.
	 */
	std::vector<CompartmentOnNeuron> get_compartments() const;


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
	// Adds configuration for single compartment.
	void add_config_compartment(
	    CompartmentOnNeuron const& compartment,
	    Neuron const& neuron,
	    Compartment::ParameterSpace const& parameter_space,
	    Environment const& environment);

	// Removes configuration for single compartment.
	void remove_config_compartment(CompartmentOnNeuron const& compartment);

	std::set<std::pair<CompartmentOnNeuron, CompartmentOnNeuron>> m_recordable_pairs;

	std::map<CompartmentOnNeuron, dapr::PropertyHolder<NumberTopBottom>> resource_map;
	NumberTopBottom m_total;
};

std::ostream& operator<<(std::ostream& os, ResourceManager const& resources) SYMBOL_VISIBLE;

} // namespace abstract
} // namespace grenade::vx::network
