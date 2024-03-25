#pragma once

#include "grenade/vx/network/abstract/detail/graph.h"
#include "grenade/vx/network/abstract/graph.h"
#include "grenade/vx/network/abstract/multicompartment_compartment.h"
#include "grenade/vx/network/abstract/multicompartment_compartment_connection/conductance.h"
#include "grenade/vx/network/abstract/multicompartment_compartment_connection_on_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_compartment_on_neuron.h"
#include <memory>

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

// forward declaration
struct Neuron;

// Make Graph visible for linker
extern template class SYMBOL_VISIBLE Graph<
    Neuron,
    detail::UndirectedGraph,
    Compartment,
    CompartmentConnection,
    CompartmentOnNeuron,
    CompartmentConnectionOnNeuron,
    std::unique_ptr>;


// Neuron uses Graph-Representation
struct GENPYBIND(visible) SYMBOL_VISIBLE Neuron
    : private Graph<
          Neuron,
          detail::UndirectedGraph,
          Compartment,
          CompartmentConnection,
          CompartmentOnNeuron,
          CompartmentConnectionOnNeuron,
          std::unique_ptr>
{
	// Constructor
	Neuron() = default;

	// Check Neuron Structure for Rulebreaks: Strongly Connected
	bool valid();

	// Add and Remove Compartment
	CompartmentOnNeuron add_compartment(Compartment const& compartment);
	void remove_compartment(CompartmentOnNeuron descriptor);

	// Add and Remove CompartmentConnection
	CompartmentConnectionOnNeuron add_compartment_connection(
	    CompartmentOnNeuron source, CompartmentOnNeuron target, CompartmentConnection const& edge);
	void remove_compartment_connection(CompartmentConnectionOnNeuron descriptor);

	// Get and Set Compartment via Compartment-ID
	Compartment const& get(CompartmentOnNeuron const& descriptor) const;
	void set(CompartmentOnNeuron const& descriptor, Compartment const& compartment);

	// Get and Set CompartmentConnection via CompartmentConnection-ID
	CompartmentConnection const& get(CompartmentConnectionOnNeuron const& descriptor) const;
	void set(
	    CompartmentConnectionOnNeuron const& descriptor, CompartmentConnection const& connection);

	// Number of Compartments and CompartmentConnections
	size_t num_compartments() const;
	size_t num_compartment_connections() const;

	// Number of in and out-going Connections of Compartment
	int in_degree(CompartmentOnNeuron const& descriptor) const;
	int out_degree(CompartmentOnNeuron const& descriptor) const;

	// Source and Target of CompartmentConnection via CompartmentConnection-ID
	CompartmentOnNeuron source(CompartmentConnectionOnNeuron const& descriptor) const;
	CompartmentOnNeuron target(CompartmentConnectionOnNeuron const& descriptor) const;

	// Iterator over all Compartments
	typedef VertexIterator CompartmentIterator;
	std::pair<CompartmentIterator, CompartmentIterator> compartment_iterators() const;

	// Checks if Neuron Contains Compartment via Compartment-ID
	bool contains(CompartmentOnNeuron const& descriptor) const;
};

} // namespace grenade::vx::network