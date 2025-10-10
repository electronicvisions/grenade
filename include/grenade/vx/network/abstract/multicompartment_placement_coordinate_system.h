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
	 * Check if any compartments are short circuited.
	 * @param x_xmax Upper limit to check.
	 */
	bool short_circuit(size_t x_max) const;

	/**
	 * Assign Compartment ID to connected(via switches) neuron-circuits.
	 * @param x x-coordinate of the neuron-circuit to assign compartment to.
	 * @param y y-coordinate of the neuron-circuit to assign compartment to.
	 * @param compartment Compartment to assign to neuron-circuit and connected circuits.
	 * @return Number of neuron circuits assigned to the compartment.
	 */
	NumberTopBottom assign_compartment_adjacent(
	    size_t x, size_t y, CompartmentOnNeuron compartment);


	// Constructor
	CoordinateSystem() = default;

	// Set and get Compartments and Configurations
	void set(int x, int y, NeuronCircuit const& neuron_circuit);
	NeuronCircuit get(int x, int y) const;
	void set_compartment(
	    int coordinate_x, int coordinate_y, CompartmentOnNeuron const& compartment);
	void set_config(int x, int y, UnplacedNeuronCircuit const& neuron_circuit_config_in);
	std::optional<CompartmentOnNeuron> get_compartment(int coordinate_x, int coordinate_y) const;
	UnplacedNeuronCircuit get_config(int x, int y) const;

	// Find coordinates of compartment
	std::vector<std::pair<int, int>> find_compartment(CompartmentOnNeuron const compartment) const;

	bool operator==(CoordinateSystem const& other) const;
	bool operator!=(CoordinateSystem const& other) const;

	std::array<NeuronCircuit, 256>& operator[](size_t y);

	std::array<std::array<NeuronCircuit, 256>, 2> coordinate_system;
};

} // namespace abstract
} // namespace grenade::vx::abstract