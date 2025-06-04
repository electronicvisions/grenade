#pragma once

#include "grenade/common/detail/graph.h"
#include "grenade/common/graph.h"
#include "grenade/vx/network/abstract/multicompartment_compartment.h"
#include "grenade/vx/network/abstract/multicompartment_compartment_connection/conductance.h"
#include "grenade/vx/network/abstract/multicompartment_compartment_connection_on_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_compartment_on_neuron.h"
#include <memory>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

// forward declaration
struct Neuron;

} // namespace abstract
} // namespace grenade::vx::network

namespace grenade::common {

// Make Graph visible for linker
extern template class SYMBOL_VISIBLE Graph<
    vx::network::abstract::Neuron,
    detail::UndirectedGraph,
    vx::network::abstract::Compartment,
    vx::network::abstract::CompartmentConnection,
    vx::network::abstract::CompartmentOnNeuron,
    vx::network::abstract::CompartmentConnectionOnNeuron,
    std::unique_ptr>;

} // namespace grenade::common

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

// Neuron uses Graph-Representation
struct GENPYBIND(inline_base("*")) SYMBOL_VISIBLE Neuron
    : private common::Graph<
          Neuron,
          common::detail::UndirectedGraph,
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

	// Clear Neuron. Removes all compartments and connections.
	void clear();

	// Number of Compartments and CompartmentConnections
	size_t num_compartments() const;
	size_t num_compartment_connections() const;

	// Number of in and out-going Connections of Compartment
	int in_degree(CompartmentOnNeuron const& descriptor) const;
	int out_degree(CompartmentOnNeuron const& descriptor) const;

	// Source and Target of CompartmentConnection via CompartmentConnection-ID
	CompartmentOnNeuron source(CompartmentConnectionOnNeuron const& descriptor) const;
	CompartmentOnNeuron target(CompartmentConnectionOnNeuron const& descriptor) const;

	// Return Iterator to begin and end of all compartments.
	typedef VertexIterator CompartmentIterator;
	std::pair<CompartmentIterator, CompartmentIterator> compartment_iterators() const;

	// Return Iterator to begin and end of all compartment-connections
	typedef EdgeIterator CompartmentConnectionIterator;
	std::pair<CompartmentConnectionIterator, CompartmentConnectionIterator>
	compartment_connection_iterators() const;

	// Return Iterator to begin and end of adjecent compartments to given compartment
	std::pair<AdjacencyIterator, AdjacencyIterator> adjacent_compartments(
	    CompartmentOnNeuron const& descriptor) const;

	/**
	 * Return mapping of compartments between this and other neuron.
	 * If no mapping between all the neurons compartments is possible an map with the possible
	 * compartment mappings is returned.
	 */
	std::map<CompartmentOnNeuron, CompartmentOnNeuron> isomorphism(Neuron const& other) const;

	/**
	 * Create a mapping of compartments if the neurons are isomorphic and the number of
	 * unmapped compartments.
	 * @param other Neuron to compare against.
	 * @param callback Callback function, that is used to write the results.
	 * @param compartment_equivalent Function to deterime whether two compartments on different
	 * neurons have equivalent properties (resource requirements).
	 */
	template <typename Callback, typename CompartmentEquivalent>
	void isomorphism(
	    Neuron const& other,
	    Callback&& callback,
	    CompartmentEquivalent&& compartment_equivalent) const;

	/**
	 *  Create a mapping of compartments of the neurons largest subneuron and the number of
	 * unmapped compartments.
	 * @param other Neuron to compare against.
	 * @param callback Callback function, that is used to write results.
	 * @param compartment_equivalent Function to determine whether two compartments on different
	 * neurons have euqivalent properites (resource requirements).
	 */
	template <typename Callback, typename CompartmentEquivalent>
	void isomorphism_subgraph(
	    Neuron const& other,
	    Callback&& callback,
	    CompartmentEquivalent&& compartment_equivalent) const;

	// Return a map of each compartment-descriptor to an index.
	std::map<CompartmentOnNeuron::Value, size_t> get_compartment_index_map() const;

	// Check if two compartments are neighbours.
	bool neighbour(CompartmentOnNeuron const& source, CompartmentOnNeuron const& target) const;

	// Check if all compartments are interconnected.
	bool compartments_connected() const;

	// Checks if neuron contains a given compartment.
	bool contains(CompartmentOnNeuron const& descriptor) const;

	/**
	 * Writes neuron topology in graphviz format.
	 * @param file File to write graphviz-graph to.
	 * @param name Name of the graph.
	 */
	void write_graphviz(std::ostream& file, std::string name);
};

std::ostream& operator<<(std::ostream& os, Neuron const& neuron);


} // namespace abstract
} // namespace grenade::vx::network

#include "grenade/vx/network/abstract/multicompartment_neuron.tcc"