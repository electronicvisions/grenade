#pragma once

#include "grenade/vx/network/abstract/multicompartment_environment.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism/synaptic_current.h"
#include "grenade/vx/network/abstract/multicompartment_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_synaptic_input_environment.h"
#include <queue>
#include <random>

namespace grenade::vx::network::abstract {

/**
 * Return type of the neuron generator.
 * The generated neuron requires an environment which specifies the synaptic inputs on the neurons
 * compartments.
 */
struct SYMBOL_VISIBLE NeuronWithEnvironment
{
	Neuron neuron;
	Environment environment;
};


/**
 * Neuron generator to create a randomly structured and sized neuron and its environment, which
 * specifies the synaptic inputs on the neuron.
 */
struct SYMBOL_VISIBLE NeuronGenerator
{
	/**
	 * Generate an empty neuron.
	 */
	NeuronWithEnvironment generate();

	/**
	 * Generate a neuron with the given number of compartments and connections.
	 *
	 * @param num_compartments Number of compartments.
	 * @param num_compartment_connections Number of compartment connections.
	 * @param limit_synaptic_input Upper limit for the number of synaptic inputs per compartment.
	 * @param top_bottom_requirements Wether the synaptic inputs are specific to the top or the
	 * bottom of the chip.
	 * @param filter_loops Wether only neurons without loops should be created.
	 */
	NeuronWithEnvironment generate(
	    size_t num_compartments,
	    size_t num_compartment_connections,
	    size_t limit_synaptic_input,
	    bool top_bottom_requirements,
	    bool filter_loops);

private:
	/**
	 *Check if generated neuron contains cycles.
	 */
	bool cyclic(Neuron const& neuron) const;
};

} // namespace grenade::vx::network::abstract