#pragma once

#include "dapr/property_holder.h"
#include "grenade/common/compartment_on_neuron.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_environment.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_on_compartment.h"
#include <map>
#include <set>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

struct GENPYBIND(visible) SYMBOL_VISIBLE Environment
{
	/**
	 * Add information about number of synaptic inputs on a neuron.
	 * @param compartment Compartment for which synaptic input is added.
	 * @param mechanism Mechanism on compartment identifier for which to add environment for
	 * @param environment Environment to add
	 */
	void add(
	    grenade::common::CompartmentOnNeuron const& compartment,
	    MechanismOnCompartment const& mechanism,
	    MechanismEnvironment const& environment);

	/**
	 * Add information about number of synaptic inputs on a neuron.
	 * @param compartment Compartment for which synaptic input is added.
	 * @param environment Environment per mechanism on compartment to add
	 */
	void add(
	    grenade::common::CompartmentOnNeuron const& compartment,
	    std::map<MechanismOnCompartment, dapr::PropertyHolder<MechanismEnvironment>> const&
	        environment);

	/**
	 * Get all synaptic inputs defined on a compartment.
	 * @param compartment Compartment for which to get the synaptic input information.
	 */
	std::map<MechanismOnCompartment, dapr::PropertyHolder<MechanismEnvironment>> get(
	    grenade::common::CompartmentOnNeuron const& compartment) const;

	/**
	 * Add a pair of recordable compartments.
	 *
	 * If only single compartments are recoreded nothing must be added here.
	 *
	 * @param compartment_a One compartment to be part of the recordable pair.
	 * @param compartment_b Other compartment to be part of the recordable pair.
	 */
	void add_recordable(
	    grenade::common::CompartmentOnNeuron const& compartment_a,
	    grenade::common::CompartmentOnNeuron const& compartment_b);

	/**
	 * Get list of all compartments that can be recorded together with the given compartment.
	 *
	 * @param compartment Compartment for which compartments which can be recorded with it are
	 * returned.
	 */
	std::vector<grenade::common::CompartmentOnNeuron> get_recordable(
	    grenade::common::CompartmentOnNeuron const& compartment) const;

	/**
	 * Get list of all compartment pairs that can be recorded.
	 */
	std::set<std::pair<grenade::common::CompartmentOnNeuron, grenade::common::CompartmentOnNeuron>>
	get_recordable_pairs() const;


private:
	std::map<
	    grenade::common::CompartmentOnNeuron,
	    std::map<MechanismOnCompartment, dapr::PropertyHolder<MechanismEnvironment>>>
	    m_synaptic_connections;

	// Holds information about the MADC recordable pairs of compartments on a neuron.
	std::set<std::pair<grenade::common::CompartmentOnNeuron, grenade::common::CompartmentOnNeuron>>
	    recordable_pairs;
};

} // namespace abstract
} // namespace grenade::vx::network
