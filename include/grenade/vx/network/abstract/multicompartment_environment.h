#pragma once

#include "dapr/property_holder.h"
#include "grenade/vx/network/abstract/multicompartment_compartment_on_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_synaptic_input_environment.h"
#include <map>
#include <set>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) SYMBOL_VISIBLE Environment
{
	/**
	 * Add information about number of synaptic inputs on a neuron.
	 * @param compartment Compartment for which synaptic input is added.
	 * @param synaptic_input Type, Number and hemisphere of the synaptic inputs on the compartment.
	 */
	void add(
	    CompartmentOnNeuron const& compartment, SynapticInputEnvironment const& synaptic_input);

	/**
	 * Add information about number of synaptic inputs on a neuron.
	 * @param compartment Compartment for which synaptic input is added.
	 * @param synaptic_inputs Multiple synaptic inputs containing information about the type, number
	 * and hemisphere of the synaptic inputs.
	 */
	void add(
	    CompartmentOnNeuron const& compartment,
	    std::vector<dapr::PropertyHolder<SynapticInputEnvironment>> const& synaptic_inputs);

	/**
	 * Get all synaptic inputs defined on a compartment.
	 * @param compartment Compartment for which to get the synaptic input information.
	 */
	std::vector<dapr::PropertyHolder<SynapticInputEnvironment>> get(
	    CompartmentOnNeuron const& compartment) const;

	/**
	 * Add a pair of recordable compartments.
	 *
	 * If only single compartments are recoreded nothing must be added here.
	 *
	 * @param compartment_a One compartment to be part of the recordable pair.
	 * @param compartment_b Other compartment to be part of the recordable pair.
	 */
	void add_recordable(
	    CompartmentOnNeuron const& compartment_a, CompartmentOnNeuron const& compartment_b);

	/**
	 * Get list of all compartments that can be recorded together with the given compartment.
	 *
	 * @param compartment Compartment for which compartments which can be recorded with it are
	 * returned.
	 */
	std::vector<CompartmentOnNeuron> get_recordable(CompartmentOnNeuron const& compartment) const;

	/**
	 * Get list of all compartment pairs that can be recorded.
	 */
	std::set<std::pair<CompartmentOnNeuron, CompartmentOnNeuron>> get_recordable_pairs() const;


private:
	std::map<CompartmentOnNeuron, std::vector<dapr::PropertyHolder<SynapticInputEnvironment>>>
	    m_synaptic_connections;

	// Holds information about the MADC recordable pairs of compartments on a neuron.
	std::set<std::pair<CompartmentOnNeuron, CompartmentOnNeuron>> recordable_pairs;
};

} // namespace abstract
} // namespace grenade::vx::network
