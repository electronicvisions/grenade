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
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {
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

	/**
	 * Get intermediate placement results
	 *
	 * Return history of placement results.
	 */
	std::vector<AlgorithmResult> get_results() const;

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
	 * @param neighbours Neighbours of the compartment from which a connection is
	 *                   searched.
	 * @param direction Direction to search in.
	 * @param search_block If a block should be searched, i.e. only spots where the
	 *                     top and bottom are free are returned.
	 */
	std::vector<PlacementSpot> find_free_spots(
	    CoordinateSystem const& coordinates,
	    size_t x_start,
	    size_t y,
	    std::set<CompartmentOnNeuron> const& neighbours,
	    int direction = 1,
	    bool search_block = false);

	/**
	 * Finds all free spots next to a placed compartment.
	 * @param coordiantes Current configuration.
	 * @param compartment Compartment to search free spots next to.
	 * @param neighbours Neighbours of `compartment`.
	 * @param search_block If a block should be searched, i.e. only spots where the
	 *                     top and bottom are free are returned.
	 */
	std::vector<PlacementSpot> find_free_spots(
	    CoordinateSystem const& coordinates,
	    CompartmentOnNeuron const& compartment,
	    std::set<CompartmentOnNeuron> const& neighbours,
	    bool search_block = false);


	/**
	 * Select a sufficiently large free spot.
	 *
	 * First, spots which are too small are disregarded.
	 * We first order spots based on the y coordinate. Here, we count
	 * the number of connections of the parent compartment to the shared line and
	 * prefer the row in which more connections are placed. This aims to only
	 * block one shared line (for later placements) if possible.
	 * On equal number of connections to the shared line, we order the spots by
	 * distance to the parent compartment (we prefer shorter distances) and next
	 * by the available space (we prefer bigger spaces).
	 * @param spots Free spots.
	 * @param min_spot_size Spot size required.
	 * @param coordinates Current state of the coordinate system.
	 * @param parent_compartment Parent of the compartment for which a free spot
	 * 		                     is selected.
	 */
	PlacementSpot select_free_spot(
	    std::vector<PlacementSpot> spots,
	    NumberTopBottom const& min_spot_size,
	    CoordinateSystem& coordinates,
	    CompartmentOnNeuron const& parent_compartment);

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
	    NumberTopBottom required_resources,
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
	    NumberTopBottom required_resources,
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
	    NumberTopBottom required_resources,
	    size_t x_start,
	    size_t y,
	    CompartmentOnNeuron const& compartment,
	    bool virtually = false);

	/**
	 * Place multiple compartment that are connected as a chain.
	 * @param coordinates Current configuration. (Or empty for virtual placement)
	 * @param neuron Neuron to be placed.
	 * @param resources Resources required by compartments of neuron.
	 * @param spot Spot where the chain should be placed.
	 * @param chain List of chain compartments.
	 * @param virtually Dummy placement to determine required spot size.
	 */
	NumberTopBottom place_chain(
	    CoordinateSystem& coordinates,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    PlacementSpot const& spot,
	    std::vector<CompartmentOnNeuron> const& chain,
	    bool virtually = false);

	/**
	 * Place a single compartment from which the morphology branches.
	 *
	 * @param coordinates Current configuration. (Or empty for virtual placement)
	 * @param neuron Neuron to be placed.
	 * @param resources Resources required by compartments of neuron.
	 * @param compartment Compartment which should be placed.
	 * @param parent Adjacent compartment which has already been placed and from
	 *               which a connection to `compartment` is established.
	 */
	NumberTopBottom place_branching_compartment(
	    CoordinateSystem& coordinates,
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    CompartmentOnNeuron const& compartment,
	    CompartmentOnNeuron const& parent);

	/**
	 * Connect all neuron circuits belonging to one compartment directly.
	 * @param coordinates Current configuration.
	 * @param compartment Compartment to be connected.
	 */
	void connect_self(CoordinateSystem& coordinates, CompartmentOnNeuron const& compartment);

	/**
	 * Logs information about latest placement.
	 * @param coordinates Current configuration.
	 * @param compartment Compartment to log placement information about.
	 */
	void output_placed(
	    CoordinateSystem const& coordinates, CompartmentOnNeuron const& compartment) const;

	// List of IDs of placed compartments
	std::vector<CompartmentOnNeuron> m_placed_compartments;
	// Results
	std::vector<AlgorithmResult> m_results;


	log4cxx::LoggerPtr m_logger;
};

} // namespace abstract
} // namespace greande::vx::network
