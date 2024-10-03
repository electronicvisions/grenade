#pragma once

#include "grenade/vx/network/abstract/multicompartment_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_neuron_circuit.h"
#include <array>
#if defined(__GENPYBIND__) or defined(__GENPYBIND_GENERATED__)
#include <pybind11/stl.h>
#endif

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * A 2d-grid representing the neuron circuits on the BSS-2-chip. Connections between neuron circuits
 * can be formed. Used in the placement algorithms for multicompartment neurons.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) CoordinateSystem
{
	/**
	 * Check for any connection to neuron-circuit at (x,y).
	 * @param x x-coordinate of the neuron-circuit.
	 * @param y y-coordinate of the neuron-circuit.
	 */
	bool connected(size_t x, size_t y) const;

	/**
	 * Check for connection to left neighbor via conductance.
	 * @param x x-coordinate of the neuron-circuit.
	 * @param y y-coordinate of the neuron-circuit.
	 */
	bool connected_left_shared(size_t x, size_t y) const;

	/**
	 * Check for connection to right neighbor via conductance.
	 * @param x x-coordinate of the neuron-circuit.
	 * @param y y-coordinate of the neuron-circuit.
	 */
	bool connected_right_shared(size_t x, size_t y) const;

	/**
	 * Check for direct connection to neighbor in opposite row.
	 * @param x x-coordinate of the neuron-circuit.
	 * @param y y-coordinate of the neuron-circuit.
	 */
	bool connected_top_bottom(size_t x, size_t y) const;

	/**
	 * Check for direct connection to right neighbor.
	 * @param x x-coordinate of the neuron-circuit.
	 * @param y y-coordinate of the neuron-circuit.
	 */
	bool connected_right(size_t x, size_t y) const;

	/**
	 * Check for direct connection to left neighbor.
	 * @param x x-coordinate of the neuron-circuit.
	 * @param y y-oordinate of the neuron-circuit.
	 */
	bool connected_left(size_t x, size_t y) const;

	/**
	 * Check for connections with open ends.
	 * @param x_xmax Upper limit to check.
	 */
	bool has_empty_connections(size_t x_max) const;

	/**
	 * Check for connections to shared line both direct and via conductance.
	 * @param x_xmax Upper limit to check.
	 */
	bool has_double_connections(size_t x_max) const;

	/**
	 * Clear connections with open ends.
	 * @param x_xmax Upper limit to check.
	 */
	void clear_empty_connections(size_t x_max);

	/**
	 * Clear empty and double connections in given part of coordinate system.
	 * Double connections occur if a neuron circuit connects to the shared line directly and via a
	 * resistor at the same time. Both connections are deleted then.
	 * @param x_max Upper limit to check.
	 */
	void clear_invalid_connections(size_t x_max);

	/**
	 * Check if any compartments are short circuited.
	 * @param x_xmax Upper limit to check.
	 */
	bool short_circuit(size_t x_max) const;

	/**
	 * Return coordinates of all neuron circuits directly connected to the given one via the shared
	 * line.
	 * @param x X-coordinate of the neuron circuit to check for connections.
	 * @param y Y-coordinate of the neuron circuit to check for connections.
	 */
	std::vector<std::pair<size_t, size_t>> connected_shared_short(size_t x, size_t y) const;

	/**
	 * Return coordinates of all neuron circuits connected to the given one via the shared
	 * line. The check only includes those connections starting from the given neuron circuit with a
	 * direct connection to the shared line and ending at another neuron circuit with a resistor.
	 * @param x X-coordinate of the neuron circuit to check for connections.
	 * @param y Y-coordinate of the neuron circuit to check for connections.
	 */
	std::vector<std::pair<size_t, size_t>> connected_shared_conductance(size_t x, size_t y) const;

	/**
	 * Assign Compartment ID to connected(via switches) neuron-circuits.
	 * @param x x-coordinate of the neuron-circuit to assign compartment to.
	 * @param y y-coordinate of the neuron-circuit to assign compartment to.
	 * @param compartment Compartment to assign to neuron-circuit and connected circuits.
	 * @return Number of neuron circuits assigned to the compartment.
	 */
	NumberTopBottom assign_compartment_adjacent(
	    size_t x, size_t y, CompartmentOnNeuron const& compartment);

	/**
	 * Check for compartment wether it has neuron circuits of each parity.
	 */
	std::pair<bool, bool> parity(CompartmentOnNeuron const& compartment) const;

	/**
	 * Return set of all compartments with neuron circuits in even columns.
	 */
	std::set<CompartmentOnNeuron> even_parity() const;
	/**
	 * Returns set of all compartments with neuron circuits in odd columns.
	 */
	std::set<CompartmentOnNeuron> odd_parity() const;


	// Constructor
	CoordinateSystem() = default;

	// Set and get Compartments and Configurations
	void set(size_t x, size_t y, NeuronCircuit const& neuron_circuit);
	NeuronCircuit get(size_t x, size_t y) const;
	void set_compartment(
	    size_t coordinate_x, size_t coordinate_y, CompartmentOnNeuron const& compartment);
	void set_config(size_t x, size_t y, UnplacedNeuronCircuit const& neuron_circuit_config_in);
	std::optional<CompartmentOnNeuron> get_compartment(
	    size_t coordinate_x, size_t coordinate_y) const;
	UnplacedNeuronCircuit get_config(size_t x, size_t y) const;

	// Clears coordiante system to empty coordinate system
	void clear();

	// Find coordinates of compartment
	std::vector<std::pair<int, int>> find_compartment(CompartmentOnNeuron const compartment) const;

	bool operator==(CoordinateSystem const& other) const;
	bool operator!=(CoordinateSystem const& other) const;

	std::array<NeuronCircuit, 256>& operator[](size_t y);

	std::array<std::array<NeuronCircuit, 256>, 2> coordinate_system;
};

} // namespace abstract
} // namespace grenade::vx::abstract