#pragma once

#include "grenade/vx/network/abstract/multicompartment_placement_algorithm.h"
#include <set>


namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) SYMBOL_VISIBLE PlacementAlgorithmRuleset : public PlacementAlgorithm
{
	PlacementAlgorithmRuleset();

	/**
	 * Executes placement algorithm.
	 * @param coordinate_system Initial state of coordinate system.
	 * @param neuron Target neuron to be placed.
	 * @param resources Resource requirements of the target neuron.
	 */
	AlgorithmResult run(
	    CoordinateSystem const& coordinate_system,
	    Neuron const& neuron,
	    ResourceManager const& resources);

	/**
	 * Clone the algorithm. Only clones the initial state of the algorithm. New algorithm is in
	 * state as after reset(). Since PlacementAlgorithm is the abstract base class the algorithm is
	 * passed as a pointer. This allows to pass the algorithm polymorphically to functions.
	 * @return Unique pointer to copy of the algorithm.
	 */
	std::unique_ptr<PlacementAlgorithm> clone() const;

	/**
	 * Reset all members of the algorithm. Used during testing.
	 */
	void reset();

private:
	/**
	 * Advances the algorithm one step.
	 * In each step a compartment or a more complexe structure (bridge, chain) is placed.
	 * @param coordinate_system State of the coordinate system at start of step.
	 * @param neuron Target neuron to be placed.
	 * @param resources Resource requirements of the target neuron.
	 * @param step Step number.
	 */
	AlgorithmResult run_one_step(
	    CoordinateSystem const& coordinate_system,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    size_t step);

	/**
	 * Return compartment descriptor of compartment with largest number of connections.
	 * @param neuron Neuron to check for compartments.
	 */
	CompartmentOnNeuron find_max_deg(Neuron const& neuron) const;

	/**
	 * Find limits of placement spots of a placed compartment.
	 * @param coordinate_system Current configuration of the coordinate system.
	 * @param compartment Compartment descriptor of the compartment to search for.
	 */
	CoordinateLimits find_limits(
	    CoordinateSystem const& coordinate_system, CompartmentOnNeuron const& compartment) const;

	/**
	 * Recursively iterates a chain of compartments and returns the lenght of the chain.
	 * @param neuron Neuron over which compartments is iterated.
	 * @param resources Resource requirements of the neuron.
	 * @param target Compartment where search starts.
	 * @param marked_compartments Set of compartments already visited.
	 */
	int iterate_compartments_rec(
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    CompartmentOnNeuron const& target,
	    std::set<CompartmentOnNeuron>& marked_compartments) const;

	/**
	 * Adds resource requirements additional to the requirements of the compartments defined by
	 their mechanisms. Is triggered if during the placement more space neeeds to be occupied by the
	 compartment to allow all its connections to be formed.
	 * @param coordinates Current coordinate system configuration.
	 * @param x x-coordiante of the compartment for which a additional requirement should be added.
	 * @param y y-coordiante of the compartment for which a additional requirement should be added.
	 */
	void add_additional_resources(CoordinateSystem const& coordinates, int x, int y);

	/**
	 * Places a single neuron compartments neuron circuits in one row to the right from the starting
	 * coordinate. If the synaptic inputs on the compartment require the cirucits in either the
	 * top or the bottom row the requested row is ignored.
	 * @param coordinate_system Current state of the coordinate system.
	 * @param neuron Target neuron.
	 * @param resources Resource reqirements of the target neuron.
	 * @param x_start The compartment is placed to the right of this x-coordinate.
	 * @param y The compartment is placed at this y-coordinate.
	 * @param compartment Compartment to be placed.
	 */
	void place_simple_right(
	    CoordinateSystem& coordinate_system,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    int x_start,
	    int y,
	    CompartmentOnNeuron const& compartment);

	/**
	 * Places a single neuron compartments neuron circuits in one row to the left from the starting
	 * coordinate. If the synaptic inputs on the compartment require the cirucits in either the
	 * top or the bottom row the requested row is ignored.
	 * @param coordinate_system Current state of the coordinate system.
	 * @param neuron Target neuron.
	 * @param resources Resource reqirements of the target neuron.
	 * @param x_start The compartment is placed to the left of this x-coordinate.
	 * @param y The compartment is placed at this y-coordinate.
	 * @param compartment Compartment to be placed.
	 */
	void place_simple_left(
	    CoordinateSystem& coordinate_system,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    int x_start,
	    int y,
	    CompartmentOnNeuron const& compartment);

	/**
	 * Place a bridge like structure which can interconnect a large number of compartments.
	 * The structure is placed from the given start x-coordiante to the right.
	 *
	 * At the given x-coordinate neuron circuits are assigned in the top row.
	 * At the left-most and right-most circuits neuron circuits in the bottom
	 * row are also assigned. This resembles a bridge like structure. For example:
	 *
	 * top:    x-x-x-x-x-x
	 * bottom: x         x
	 * @param coordinate_system Configuration of the coordinate system.
	 * @param neuron Target neuron.
	 * @param resources Resource requirements of the target neuron.
	 * @param x_start x-coordinate where bridge structure starts to be placed.
	 * @param compartment Compartment to be placed as a bridge structure.
	 */
	void place_bridge_right(
	    CoordinateSystem& coordinate_system,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    int x_start,
	    CompartmentOnNeuron const& compartment);

	/**
	 * Place a bridge like structure which can interconnect a large number of compartments.
	 * The structure is placed from the given start x-coordinate to the left.
	 *
	 * At the given x-coordinate neuron circuits are assigned in the top row.
	 * At the left-most and right-most circuits neuron circuits in the bottom
	 * row are also assigned. This resembles a bridge like structure. For example:
	 *
	 * top:    x-x-x-x-x-x
	 * bottom: x         x
	 * @param coordinate_system Configuration of the coordinate system.
	 * @param neuron Target neuron.
	 * @param resources Resource requirements of the target neuron.
	 * @param x_start x-coordinate where bridge structure starts to be placed.
	 * @param compartment Compartment to be placed as a bridge structure.
	 */
	void place_bridge_left(
	    CoordinateSystem& coordinate_system,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    int x_start,
	    CompartmentOnNeuron const& compartment);

	/**
	 * Interconnect all neuron circuits that belong to the specified compartment.
	 * @param coordianate_system Coordinate system on which to perform the connections.
	 * @param compartment Compartment whichs circuits should be connected.
	 */
	void connect_self(CoordinateSystem& coordinate_system, CompartmentOnNeuron const& compartment);

	/**
	 * Connect two adjacently placed compartments with each other via conductance.
	 * @param coordinate_system Coordinate system on which to perform the connection.
	 * @param neuron Target neuron.
	 * @param y y-coordiante of the coordinate system in which the connection should be established.
	 * @param compartment_a Source compartment of the connection.
	 * @param compartment_b Target compartment of the connection.
	 */
	void connect_next(
	    CoordinateSystem& coordinate_system,
	    Neuron const& neuron,
	    int y,
	    CompartmentOnNeuron const& compartment_a,
	    CompartmentOnNeuron const& compartment_b);

	/**
	 * Interconnect a leaf connected to a compartment.
	 * @param coordinate_system Current configuration of the coordinate system.
	 * @param neuron Target neuron.
	 * @param compartment_leaf Leaf compartment to connect to node compartment.
	 * @param compartment_node Node compartment to which leaf is connected.
	 */
	AlgorithmResult connect_leafs(
	    CoordinateSystem& coordinate_system,
	    Neuron const& neuron,
	    CompartmentOnNeuron const& compartment_leaf,
	    CompartmentOnNeuron const& compartment_node);

	/**
	 * Connect all compartments placed on the coordinate system.
	 * @param coordinate_system Configuration of the coordinate system.
	 * @param neuron Target neuron.
	 */
	void connect_all(CoordinateSystem& coordinate_system, Neuron const& neuron);

	// List of IDs of placed compartments
	std::vector<CompartmentOnNeuron> placed_compartments;
	// List of additional required resources
	std::map<std::optional<CompartmentOnNeuron>, NumberTopBottom> additional_resources;
	std::vector<std::optional<CompartmentOnNeuron>> additional_resources_compartments;
	// Results
	std::vector<AlgorithmResult> m_results;
};

} // namespace abstract
} // namespace greande::vx::network