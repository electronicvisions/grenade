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
	// Adds information about number of synaptic inputs on a neuron.
	void add(
	    CompartmentOnNeuron const& compartment, SynapticInputEnvironment const& synaptic_input);
	void add(
	    CompartmentOnNeuron const& compartment,
	    std::vector<dapr::PropertyHolder<SynapticInputEnvironment>> const& synaptic_inputs);

	std::vector<dapr::PropertyHolder<SynapticInputEnvironment>> get(
	    CompartmentOnNeuron const& compartment) const;

	// Adds a pair of recordable compartments to the list if only a single compartment is recorded
	// nothing must be add here.
	void add_recordable(
	    CompartmentOnNeuron const& compartment_a, CompartmentOnNeuron const& compartment_b);

	// Gets all compartments that can be recorded with the given compartment.
	std::vector<CompartmentOnNeuron> get_recordable(CompartmentOnNeuron const& compartment) const;

	// Returns set of all recordable pairs.
	std::set<std::pair<CompartmentOnNeuron, CompartmentOnNeuron>> get_recordable_pairs() const;


private:
	std::map<CompartmentOnNeuron, std::vector<dapr::PropertyHolder<SynapticInputEnvironment>>>
	    m_synaptic_connections;

	// Holds information about the MADC recordable pairs of compartments on a neuron.
	std::set<std::pair<CompartmentOnNeuron, CompartmentOnNeuron>> recordable_pairs;
};

} // namespace abstract
} // namespace grenade::vx::network
