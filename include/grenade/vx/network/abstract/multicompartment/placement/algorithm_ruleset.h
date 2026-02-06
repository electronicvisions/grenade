#pragma once

#include "grenade/vx/network/abstract/multicompartment/placement/algorithm.h"
#include "grenade/vx/network/abstract/multicompartment/placement/placement_spot.h"
#include <algorithm>
#include <set>

namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx


namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {
/**
 * Placement algorithm for multicopartmen neurons.
 *
 * This algorithms follows a set of rules and places a logical neuron on a virtual coordinate system
 * by iteratively placing its compartments.
 *
 * The first compartment is placed at the center of the coordinate system to allow for maximum
 * flexibility in placement direction.
 * For each placed compartment, all of its neighbour compartments are placed by searching for free
 * spots on the coordinate system that can be interconneced with the placed compartment.
 * The algorithm follows a depth first pattern for the placement and prioritizes the placement of
 * smaller structures over more complex structures.
 *
 * In the current state the algorithm does not support cyclic neuron topologies.
 */
struct GENPYBIND(visible) SYMBOL_VISIBLE PlacementAlgorithmRuleset : public PlacementAlgorithm
{
	PlacementAlgorithmRuleset();

	/**
	 * Constructs Placement-Algorithm with specified placement pattern.
	 * For depth_search_first enabled the neuron is placed in depth first starting from the
	 * largest compartment. Elsewise the neuron is placed in breadth first.
	 * @param depth_search_first Wether the neuron should be placed in depth first.
	 */
	PlacementAlgorithmRuleset(bool depth_search_first);

	/**
	 * Executes placement algorithm.
	 * @param coordinate_system Initial state of coordinate system.
	 * @param neuron Target neuron to be placed.
	 * @param resources Resource requirements of the target neuron.
	 */
	AlgorithmResult run(
	    CoordinateSystem const& coordinates,
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
	    Neuron const& neuron, ResourceManager const& resources, size_t step);

	/**
	 * Finds all lower and upper limits of a placed compartment.
	 * @param coordinates Current configuration.
	 * @param compartment Compartment for which limits are searched.
	 */
	CoordinateLimits find_limits(
	    CoordinateSystem const& coordinates, CompartmentOnNeuron const& compartment) const;

	/**
	 * Finds all free spots.
	 * @param coordiantes Current configuration.
	 * @param x_start Column to start search at.
	 * @param y Row to search in.
	 * @param direction Direction to search in.
	 */
	std::vector<PlacementSpot> find_free_spots(
	    CoordinateSystem const& coordinates, size_t x_start, size_t y, int direction = 1);

	/**
	 * Finds all free spots next to a placed compartment.
	 * @param coordiantes Current configuration.
	 * @param compartment Compartment to search free spots next to.
	 */
	std::vector<PlacementSpot> find_free_spots(
	    CoordinateSystem const& coordinates, CompartmentOnNeuron const& compartment);


	/**
	 * Select a sufficiently large free spot.
	 * @param spots Free spots.
	 * @param min_spot_size Spot size required.
	 * @param closest Prioritize closer spots.
	 * @param largest Prioritize larger spots.
	 * @param block Only consider rectangluar spots.
	 */
	PlacementSpot select_free_spot(
	    std::vector<PlacementSpot> spots,
	    NumberTopBottom const& min_spot_size,
	    bool closest = true,
	    bool largest = false,
	    bool block = false);

	/**
	 * Determine unplaced neighbours
	 * @param neuron Neuron to be placed.
	 * @param compartment Compartment whichs neighbours are to be determined.
	 */
	std::set<CompartmentOnNeuron> unplaced_neighbours(
	    Neuron const& neuron, CompartmentOnNeuron const& compartment) const;

	/**
	 * Return compartment descriptor of compartment with largest number of connections.
	 * @param neuron Neuron to check for compartments.
	 */
	CompartmentOnNeuron find_max_deg(Neuron const& neuron) const;

	/**
	 * Simple placement. Place in one row if other row is not explicitly requested.
	 * @param coordiantes Current configuration. (Or empty for virutal placement)
	 * @param neuron Neuron to be placed.
	 * @param resources Requried resources by compartments of neuron.
	 * @param x_start Column to start placement.
	 * @param y Row to start placement.
	 * @param compartment Compartment to be placed.
	 * @param direction Direction of placement.
	 * @param virtually Dummy placement to determine required spot size.
	 * @return Number of allocated neuron circuits.
	 */
	NumberTopBottom place_simple(
	    CoordinateSystem& coordinates,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    size_t x_start,
	    size_t y,
	    CompartmentOnNeuron const& compartment,
	    int direction = 1,
	    bool virtually = false);

	/**
	 *  Use simple placement to place compartment to the right. See place_right for more
	 * information.
	 * */
	NumberTopBottom place_simple_right(
	    CoordinateSystem& coordinates,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    size_t x_start,
	    size_t y,
	    CompartmentOnNeuron const& compartment,
	    bool virtually = false);

	/**
	 *  Use simple placement to place compartment to the left. See place_right for more
	 * information.
	 * */
	NumberTopBottom place_simple_left(
	    CoordinateSystem& coordinates,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    size_t x_start,
	    size_t y,
	    CompartmentOnNeuron const& compartment,
	    bool virtually = false);

	/**
	 * Place a bridge like structure which can interconnect a large number of compartments.
	 *
	 * At the given x-coordinate neuron circuits are assigned in the top row.
	 * At the left-most and right-most circuits neuron circuits in the bottom
	 * row are also assigned. This resembles a bridge like structure. For example:
	 *
	 * top    x-x-x-x-x-x
	 * bottom x         x
	 * @param coordinates Current configuration. (Or empty for virutal placement)
	 * @param neuron Neuron to be placed.
	 * @param resources Required resources by compartments of neuron.
	 * @param x_start Column to start placement from.
	 * @param compartment Compartment to be placed as a bridge.
	 * @param direction Direction of placement.
	 * @param virtually Dummy placement to determine required spot size.
	 * @return Number of allocated neuron circuits.
	 */
	NumberTopBottom place_bridge(
	    CoordinateSystem& coordinates,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    size_t x_start,
	    CompartmentOnNeuron const& compartment,
	    int direction = 1,
	    bool virtually = false);

	/**
	 * Place a bridge structure to the right. See place_bridge for more information.
	 */
	NumberTopBottom place_bridge_right(
	    CoordinateSystem& coordinates,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    size_t x_start,
	    CompartmentOnNeuron const& compartment,
	    bool virtually = false);

	/**
	 * Place a bridge structure to the left. See place_bridge for more information.
	 */
	NumberTopBottom place_bridge_left(
	    CoordinateSystem& coordinates,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    size_t x_start,
	    CompartmentOnNeuron const& compartment,
	    bool virtually = false);


	/**
	 * Place multiple leafs.
	 * @param coordinates Current configuration. (Or empty for virutal placement)
	 * @param neuron Neuron to be placed.
	 * @param resources Resources required by comaprtment of neuron.
	 * @param x Column to start placement.
	 * @param y Row to start placement.
	 * @param compartment Compartment whichs leafs are placed.
	 * @param leafs List of leaf compartments.
	 * @param direction Direction of placement.
	 * @param virtually Dummy placement to determine required spot size.
	 */
	NumberTopBottom place_leafs(
	    CoordinateSystem& coordinates,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    size_t x,
	    size_t y,
	    CompartmentOnNeuron compartment,
	    std::vector<CompartmentOnNeuron> const& leafs,
	    int direction = 1,
	    bool virtually = false);

	/**
	 * Place multiple compartment that are connected as a chain.
	 * @param coordinates Current configuration. (Or empty for virtual placement)
	 * @param neuron Neuron to be placed.
	 * @param resources Resources required by compartments of neuron.
	 * @param x Column to start placement from.
	 * @param y Row to start placement from.
	 * @param chain List of chain compartments.
	 * @param direction Direction of placement.
	 * @param virtually Dummy placement to determine required spot size.
	 */
	NumberTopBottom place_chain(
	    CoordinateSystem& coordinates,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    size_t x,
	    size_t y,
	    std::vector<CompartmentOnNeuron> const& chain,
	    int direction = 1,
	    bool virtually = false);

	/**
	 * Place a branch of compartments. This can include simple placements and bridge placements.
	 * @param coordinates Current configuration. (Or empty for virtual placement)
	 * @param neuron Neuron to be placed.
	 * @param resources Resources required by compartments of neuron.
	 * @param search_start Adjacent compartment from where on free space is searched.
	 * @param branch List of compartments which should be placed. The compartments have to form
	 * a branch for the algorithm to work
	 */
	NumberTopBottom place_branch(
	    CoordinateSystem& coordinates,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    CompartmentOnNeuron const& search_start,
	    std::vector<CompartmentOnNeuron> const& branch);

	/**
	 * Connect all neuron circuits belonging to one compartment directly.
	 * @param coordinates Current configuration.
	 * @param compartment Compartment to be connected.
	 */
	void connect_self(CoordinateSystem& coordinates, CompartmentOnNeuron const& compartment);

	/**
	 * Connect two compartments that are directly adjacent.
	 * @param coordinates Current configuration.
	 * @param neuron Neuron to be placed.
	 * @param compartment_a One compartment to be connected.
	 * @param compartment_b Other compartment to be connected.
	 */
	void connect_adjacent(
	    CoordinateSystem& coordinates,
	    Neuron const& neuron,
	    CompartmentOnNeuron const& compartment_a,
	    CompartmentOnNeuron const& compartment_b);

	/**
	 * Connect two compartment that are not directly adjacent.
	 * @param coordinates Current configuration.
	 * @param neuron Neuron to be placed.
	 * @param compartment_a One compartment to be connected.
	 * @param comaprmtent_b Other compartment to be connected.
	 */
	void connect_distant(
	    CoordinateSystem& coordinates,
	    Neuron const& neuron,
	    CompartmentOnNeuron const& compartment_a,
	    CompartmentOnNeuron const& compartment_b);

	/**
	 * Connect all compartments placed in this step to all placed neighbours.
	 * @param coordinates Current configuration.
	 * @param neuron Neuron to be placed.
	 */
	void connect_placed(CoordinateSystem& coordinates, Neuron const& neuron);

	/**
	 * Logs information about latest placement.
	 * @param coordinates Current configuration.
	 * @param compartment Compartment to log placement information about.
	 */
	void output_placed(
	    CoordinateSystem const& coordinates, CompartmentOnNeuron const& compartment) const;

	/**
	 * Helper function for trying to connect and catching the error. This makes a runtime error out
	 * of a logic error raised by the coordinate system. The error type is important for automatic
	 * placement testing, as only runtime errors are caught.
	 * @param coordinates Coordiante system to place the connection on.
	 * @param x_first First circuits x-coordinate.
	 * @param x_second Second circuits x-coordiante.
	 *@param y Y-Coordinate for connection.
	 */
	void try_connect_shared(CoordinateSystem& coordiantes, size_t x_left, size_t x_right, size_t y);


	// List of IDs of placed compartments
	std::vector<CompartmentOnNeuron> m_placed_compartments;
	// Set of fully connected compartments
	std::set<std::pair<CompartmentOnNeuron, CompartmentOnNeuron>> m_placed_connections;
	// Results
	std::vector<AlgorithmResult> m_results;


	log4cxx::LoggerPtr m_logger;

	// Wether algorithm should search first in breadth or depth.
	bool m_depth_search_first = false;
};

} // namespace abstract
} // namespace greande::vx::network