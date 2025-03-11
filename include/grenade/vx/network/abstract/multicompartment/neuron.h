#pragma once

#include "grenade/common/detail/graph.h"
#include "grenade/common/graph.h"
#include "grenade/vx/network/abstract/multicompartment/compartment.h"
#include "grenade/vx/network/abstract/multicompartment/compartment_connection/conductance.h"
#include "grenade/vx/network/abstract/multicompartment/compartment_connection_on_neuron.h"
#include "grenade/vx/network/abstract/multicompartment/compartment_on_neuron.h"
#include "grenade/vx/network/abstract/multicompartment/neighbours.h"
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

/**
 * Graph representation of a multicompartment Neuron.
 * Compartments are represented as graph vertices and connections as graph edges.
 */
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
	struct GENPYBIND(visible) ParameterSpace
	{
		struct GENPYBIND(visible) Parameterization
		{
			std::map<CompartmentOnNeuron, Compartment::ParameterSpace::Parameterization>
			    compartments;
		};

		/**
		 * Check if the given parameterization is valid for the parameter space.
		 * This checks if the parameterization for each compartment is valid.
		 * @param paramterization Parameterization to check validity for.
		 */
		bool valid(Parameterization const& parameterization) const;

		std::map<CompartmentOnNeuron, Compartment::ParameterSpace> compartments;
	};


	Neuron() = default;

	/**
	 * Copy constructors.
	 * Deleted since the descriptors of the compartments on the neuron need to be unique and
	 * therfore can not be copied.
	 */
	Neuron(Neuron const& other) = delete;
	Neuron& operator=(Neuron const& other) = delete;

	/**
	 * Move constructors.
	 */
	Neuron(Neuron&& other) = default;
	Neuron& operator=(Neuron&& other) = default;

	/**
	 * Adds a compartment to the neuron.
	 * @param compartment Compartment to add to the neuron.
	 * @return Compartment descriptor on the neuron.
	 */
	CompartmentOnNeuron add_compartment(Compartment const& compartment);

	/**
	 * Removes a compartment from the neuron.
	 * @param descriptor Descriptor of the compartment to remove.
	 */
	void remove_compartment(CompartmentOnNeuron descriptor);

	/**
	 * Add a compartment connection to the neuron.
	 * @param source Source compartment descriptor.
	 * @param target Target compartment descriptor.
	 * @param edge Connection.
	 * @return Descriptor of the connection on the neuron.
	 */
	CompartmentConnectionOnNeuron add_compartment_connection(
	    CompartmentOnNeuron source, CompartmentOnNeuron target, CompartmentConnection const& edge);

	/**
	 * Remove a connection from the neuron.
	 * @param descriptor Connecion descriptor of the connection to remove.
	 */
	void remove_compartment_connection(CompartmentConnectionOnNeuron descriptor);

	/**
	 * Get a compartment via its descriptor.
	 * @param descriptor Descriptor of the requested compartment.
	 * @return Compartment corresponding to the descriptor.
	 */
	Compartment const& get(CompartmentOnNeuron const& descriptor) const;

	/**
	 * Set a compartment via a descriptor.
	 * @param descriptor Descriptor of the compartment to be set.
	 * @param compartment Compartment to be set for the given descriptor.
	 */
	void set(CompartmentOnNeuron const& descriptor, Compartment const& compartment);

	/**
	 * Get a compartment connection via its descriptor.
	 * @param descriptor Descriptor of the requested connection.
	 * @return Connection corresponding to the descriptor.
	 */
	CompartmentConnection const& get(CompartmentConnectionOnNeuron const& descriptor) const;

	/**
	 * Set a compartment connection via a descriptor.
	 * @param descriptor Descriptor of the connection to be set.
	 * @param conection Connection to be set for the given descriptor.
	 */
	void set(
	    CompartmentConnectionOnNeuron const& descriptor, CompartmentConnection const& connection);

	/**
	 * Clear the neuron.
	 * Remove all compartments and connections.
	 */
	void clear();

	/**
	 * Return number of compartments on the neuron.
	 * @return Number of compartments on the neuron.
	 */
	size_t num_compartments() const;

	/**
	 * Return number of connections on the neuron.
	 * @return Number of connections on the neuron.
	 */
	size_t num_compartment_connections() const;

	/**
	 * Return number of connections on a compartment.
	 * Since the neuron has a undirected graph representation the ingoing and outgoing degree is
	 * equal.
	 * @param descriptor Descriptor of the compartment to give degree.
	 * @return Number of outgoing connections.
	 */
	size_t get_compartment_degree(CompartmentOnNeuron const& descriptor) const;

	/**
	 * Return compartment with largest degree.
	 * Since the neuron has a undirected graph representation the ingoing and outgoing degree is
	 * equal.
	 * @return Descriptor of the compartment with the highest degree.
	 */
	CompartmentOnNeuron get_max_degree_compartment() const;

	/**
	 * Return source of the given connection.
	 * @param descriptor Descriptor of the connection.
	 * @return Descriptor of the source compartment.
	 */
	CompartmentOnNeuron source(CompartmentConnectionOnNeuron const& descriptor) const;

	/**
	 * Return target of the given connection.
	 * @param descriptor Descriptor of the connection.
	 * @return Descriptor of the target compartment.
	 */
	CompartmentOnNeuron target(CompartmentConnectionOnNeuron const& descriptor) const;

	/**
	 * Return iterators to begin and end of all compartments.
	 * @return Pair of iterators to begin and end of all compartments.
	 */
	typedef VertexIterator CompartmentIterator;
	std::pair<CompartmentIterator, CompartmentIterator> compartments() const;

	/**
	 * Return iterators to begin and end of all connections.
	 * @return Pair of iterators to begin and end of all connections.
	 */
	typedef EdgeIterator CompartmentConnectionIterator;
	std::pair<CompartmentConnectionIterator, CompartmentConnectionIterator>
	compartment_connections() const;

	/**
	 * Return iterators to begin and end of adjacent compartments.
	 * @return Pair of iterators to begin and end of adjacent compartments.
	 */
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

	/**
	 * Return a map of each compartment-descriptor to an index.
	 * */
	std::map<CompartmentOnNeuron::Value, size_t> get_compartment_index_map() const;

	/**
	 *  Check if two compartments are neighbours.
	 */
	bool neighbour(CompartmentOnNeuron const& source, CompartmentOnNeuron const& target) const;

	/**
	 *  Check if compartment is part of a chain and return chain length if it is part of one.
	 * @param compartment Compartment to check.
	 * @param marked_compartments Visited compartments in chain.
	 * @return Length of chain. -1 if not in chain.
	 */
	int chain_length(
	    CompartmentOnNeuron const& compartment,
	    std::set<CompartmentOnNeuron>& marked_compartments) const;

	/**
	 * Return ordered list of compartments inside a chain.
	 * @param compartment Arbitrary compartment inside a chain.
	 * @param blacklist_compartment compartment not to consider for chain to prevent looping.
	 */
	std::vector<CompartmentOnNeuron> chain_compartments(
	    CompartmentOnNeuron const& compartment,
	    CompartmentOnNeuron const& blacklist_compartment = CompartmentOnNeuron()) const;

	/**
	 * Classify neighbours into parts of branches, chains or as a leaf.
	 * @param compartment Compartment whichs neighbours are beeing classified.
	 * @param neighbours_whitelist If vector is given only those compartments are classified.
	 * @return Object with compartments classified to a category.
	 */
	CompartmentNeighbours classify_neighbours(
	    CompartmentOnNeuron const& compartment,
	    std::set<CompartmentOnNeuron> neighbours_whitelist =
	        std::set<grenade::vx::network::abstract::CompartmentOnNeuron>()) const;

	/**
	 * Check if all compartments are connected.
	 */
	bool compartments_connected() const;

	/**
	 * Check if neuron contains a compartment.
	 * @param descriptor Descriptor of the compartment to check for.
	 */
	bool contains(CompartmentOnNeuron const& descriptor) const;

	/**
	 * Check if the given paramter space is valid for the neuron.
	 * This checks if the parameter space for each compartment of the neuron is valid.
	 * @param paramter_space Paramter space to check for validity.
	 */
	bool valid(ParameterSpace const& parameter_space) const;

	/**
	 * Writes neuron topology in graphviz format.
	 * @param filename File to write graphviz-graph to.
	 * @param name Name of the graph.
	 */
	void write_graphviz(std::string filename, std::string name);
};

std::ostream& operator<<(std::ostream& os, Neuron const& neuron);


} // namespace abstract
} // namespace grenade::vx::network

#include "grenade/vx/network/abstract/multicompartment/neuron.tcc"
