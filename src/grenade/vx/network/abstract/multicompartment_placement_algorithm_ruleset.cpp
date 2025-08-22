#include "grenade/vx/network/abstract/multicompartment_placement_algorithm_ruleset.h"
#include <sstream>
#include <log4cxx/logger.h>

namespace grenade::vx::network::abstract {

PlacementAlgorithmRuleset::PlacementAlgorithmRuleset() :
    m_logger(log4cxx::Logger::getLogger("grenade.MC.Placement.Ruleset"))
{
}

PlacementAlgorithmRuleset::PlacementAlgorithmRuleset(bool depth_search_first) :
    m_logger(log4cxx::Logger::getLogger("grenade.MC.Placement.Ruleset")),
    m_breadth_first_search(!depth_search_first)
{
}

AlgorithmResult PlacementAlgorithmRuleset::run(
    CoordinateSystem const&, Neuron const& neuron, ResourceManager const& resources)
{
	size_t step = 0;
	AlgorithmResult result;

	while (true) {
		result = run_one_step(neuron, resources, step);
		if (result.finished) {
			return result;
		}
		step++;
	}
}

std::unique_ptr<PlacementAlgorithm> PlacementAlgorithmRuleset::clone() const
{
	return std::make_unique<PlacementAlgorithmRuleset>();
}

void PlacementAlgorithmRuleset::reset()
{
	m_placed_compartments.clear();
	m_placed_connections.clear();
	m_results.clear();
}

// Find right and left limit of compartment placed
CoordinateLimits PlacementAlgorithmRuleset::find_limits(
    CoordinateSystem const& coordinates, CompartmentOnNeuron const& compartment) const
{
	CoordinateLimits limits;
	// Iterate over Top and Bottom Row
	for (int y = 0; y < 2; y++) {
		CoordinateLimit limit;
		bool valid_limit = false;

		bool finished = false;
		size_t lower_limit = 0;

		// Search for another limit until end of coordinate system is reached
		while (!finished) {
			lower_limit = 0;
			// Top Row Check if previous Limits exist
			if (y == 0) {
				if (!limits.top.empty()) {
					lower_limit = limits.top.back().upper + 1;
				}
			}
			// Bottom Row Check if Limits exist
			else if (y == 1) {
				if (!limits.bottom.empty()) {
					lower_limit = limits.bottom.back().upper + 1;
				}
			}

			// Find next or first Lower Limit
			for (size_t x = lower_limit; x <= 255; x++) {
				if (coordinates.coordinate_system[y][x].compartment == compartment) {
					limit.lower = x;
					valid_limit = true;
					break;
				}
				if (x == 255) {
					finished = true;
					valid_limit = false;
				}
			}
			// Find Upper Limit to Lower Limit
			if (!finished) {
				for (size_t x = limit.lower; x <= 255; x++) {
					if (x == 255) {
						finished = true;
						limit.upper = x;
						valid_limit = true;
						break;
					}
					if (coordinates.coordinate_system[y][x + 1].compartment != compartment) {
						limit.upper = x;
						valid_limit = true;
						break;
					}
				}
			}

			// Push Limit in correct Vector
			if (y == 0 && valid_limit) {
				limits.top.push_back(limit);
			} else if (y == 1 && valid_limit) {
				limits.bottom.push_back(limit);
			}
		}
	}
	return limits;
}

std::vector<PlacementSpot> PlacementAlgorithmRuleset::find_free_spots(
    CoordinateSystem const& coordinates, size_t x_start, size_t y, int direction)
{
	LOG4CXX_DEBUG(
	    m_logger, "Checking for free spots at: [x= " << x_start << ",y= " << y
	                                                 << ",dir= " << direction << "]");
	std::vector<PlacementSpot> spots;

	PlacementSpot spot;
	spot.y = y;
	spot.x = x_start;
	spot.direction = direction;
	bool spot_found = false;

	// Direction 1 (right)
	if (direction == 1) {
		// Check for free space in requested row
		for (size_t x = x_start; x < 256; x++) {
			if (coordinates.coordinate_system[y][x].switch_shared_right) {
				break;
			}
			if (!coordinates.coordinate_system[y][x].compartment) {
				// First free spot in x direction is marked
				if (!spot_found) {
					spot.x = x;
					spot.distance = std::max(x_start, x) - std::min(x_start, x);
					spot_found = true;
					spot.free_space += NumberTopBottom(1, 1 - y, y);
				} else {
					spot.free_space += NumberTopBottom(1, 1 - y, y);
				}
			}
			// Not free and already a spot found
			else if (spot_found) {
				spots.push_back(spot);
				spot_found = false;
				spot.free_space = NumberTopBottom(0, 0, 0);
			}
			if (x == 255 && spot_found) {
				spots.push_back(spot);
				spot_found = false;
				spot.free_space = NumberTopBottom(0, 0, 0);
			}
		}
		// For all found spots determine number of resources in opposite row
		for (auto& spot : spots) {
			// Check for free space in opposite row
			size_t spot_upper_limit = spot.x + spot.free_space.number_total;
			for (size_t x = spot.x; x < spot_upper_limit; x++) {
				if (!coordinates.coordinate_system[1 - y][x].compartment) {
					if (!spot.found_opposite) {
						spot.found_opposite = true;
						spot.x_opposite = x;
						spot.free_space += NumberTopBottom(1, y, 1 - y);
					} else {
						spot.free_space += NumberTopBottom(1, y, 1 - y);
					}
				}
				// Not free and already a spot found
				else if (spot.found_opposite) {
					break;
				}
			}
		}
	}
	// Direction -1 (left)
	else if (direction == -1) {
		// Check for free space in requested row
		for (size_t x = x_start; x + 1 > 0; x--) {
			if (x > 0 && coordinates.coordinate_system[y][x - 1].switch_shared_right) {
				break;
			}
			if (!coordinates.coordinate_system[y][x].compartment) {
				// First free spot in x direction is marked
				if (!spot_found) {
					spot.x = x;
					spot.distance = std::max(x_start, x) - std::min(x_start, x);
					spot_found = true;
					spot.free_space += NumberTopBottom(1, 1 - y, y);
				} else {
					spot.free_space += NumberTopBottom(1, 1 - y, y);
				}
			}
			// Not free and already a spot found
			else if (spot_found) {
				spots.push_back(spot);
				spot_found = false;
				spot.free_space = NumberTopBottom(0, 0, 0);
			}
			if (x == 0 && spot_found) {
				spots.push_back(spot);
				spot_found = false;
				spot.free_space = NumberTopBottom(0, 0, 0);
			}
		}
		// For all found spots determine number of resources in opposite row
		for (auto& spot : spots) {
			// Check for free space in opposit row
			size_t spot_lower_limit = spot.x - spot.free_space.number_total + 1;
			for (size_t x = spot.x; x + 1 > spot_lower_limit; x--) {
				if (!coordinates.coordinate_system[1 - y][x].compartment) {
					if (!spot.found_opposite) {
						spot.found_opposite = true;
						spot.x_opposite = x;
						spot.free_space += NumberTopBottom(1, y, 1 - y);
					} else {
						spot.free_space += NumberTopBottom(1, y, 1 - y);
					}
				}
				// Not free and already a spot found
				else if (spot.found_opposite) {
					break;
				}
			}
		}
	}
	// Invalid direction
	else {
		throw std::logic_error("Invalid direction.");
	}
	LOG4CXX_DEBUG(m_logger, "Found " << spots.size() << " spots.");
	return spots;
}

std::vector<PlacementSpot> PlacementAlgorithmRuleset::find_free_spots(
    CoordinateSystem const& coordinates, CompartmentOnNeuron const& compartment)
{
	CoordinateLimits limits = find_limits(coordinates, compartment);
	std::vector<PlacementSpot> spots_limit;

	// All spots found
	std::vector<PlacementSpot> spots;

	// Search for free spot in top row
	for (auto limit : limits.top) {
		spots_limit = find_free_spots(coordinates, limit.upper, 0, 1);
		spots.insert(spots.end(), spots_limit.begin(), spots_limit.end());

		spots_limit = find_free_spots(coordinates, limit.lower, 0, -1);
		spots.insert(spots.end(), spots_limit.begin(), spots_limit.end());
	}
	// Search for free spot in bottom row
	for (auto limit : limits.bottom) {
		spots_limit = find_free_spots(coordinates, limit.upper, 1, 1);
		spots.insert(spots.end(), spots_limit.begin(), spots_limit.end());

		spots_limit = find_free_spots(coordinates, limit.lower, 1, -1);
		spots.insert(spots.end(), spots_limit.begin(), spots_limit.end());
	}

	return spots;
}

PlacementSpot PlacementAlgorithmRuleset::select_free_spot(
    std::vector<PlacementSpot> spots,
    NumberTopBottom const& min_spot_size,
    bool closest,
    bool largest,
    bool block)
{
	LOG4CXX_DEBUG(
	    m_logger, "Selecting free spot (closest= " << closest << ",largest= " << largest
	                                               << ",block= " << block << ")");
	std::stringstream ss;
	ss << "Given spots:[";
	for (auto& spot : spots) {
		ss << spot;
	}
	ss << "]";
	LOG4CXX_TRACE(m_logger, ss.str());

	std::vector<PlacementSpot> large_enough_spots;
	PlacementSpot final_spot;

	// Select spots that meet resource requirements (block resources or all resouces depending on
	// block flag)
	for (auto spot : spots) {
		if ((!block && min_spot_size <= spot.free_space) ||
		    (block && min_spot_size <= spot.get_free_block_space().second)) {
			large_enough_spots.push_back(spot);
		}
	}

	size_t const count = large_enough_spots.size();
	// Maximize free space
	if (largest) {
		std::vector<PlacementSpot> largest_spots;
		PlacementSpot largest_spot;
		size_t largest_spot_index;

		while (largest_spots.size() < count) {
			NumberTopBottom size;

			for (size_t i = 0; i < large_enough_spots.size(); i++) {
				if (large_enough_spots[i].free_space > size) {
					size = large_enough_spots[i].free_space;
					largest_spot = large_enough_spots[i];
					largest_spot_index = i;
				}
			}
			largest_spots.push_back(largest_spot);
			large_enough_spots.erase(large_enough_spots.begin() + largest_spot_index);
		}
		large_enough_spots = largest_spots;
	}

	// Minimize distance
	if (closest) {
		std::vector<PlacementSpot> closest_spots;
		PlacementSpot closest_spot;
		size_t closest_spot_index;

		while (closest_spots.size() < count) {
			size_t distance_min = 256;

			for (size_t i = 0; i < large_enough_spots.size(); i++) {
				if (large_enough_spots[i].distance < distance_min) {
					distance_min = large_enough_spots[i].distance;
					closest_spot = large_enough_spots[i];
					closest_spot_index = i;
				}
			}
			closest_spots.push_back(closest_spot);
			large_enough_spots.erase(large_enough_spots.begin() + closest_spot_index);
		}
		large_enough_spots = closest_spots;
	}

	// Select first element best spot for given criteriums
	if (large_enough_spots.empty()) {
		throw std::logic_error("No valid placement spot found.");
	}
	final_spot = large_enough_spots.at(0);
	return final_spot;
}


std::set<CompartmentOnNeuron> PlacementAlgorithmRuleset::unplaced_neighbours(
    Neuron const& neuron, CompartmentOnNeuron const& compartment) const
{
	std::set<CompartmentOnNeuron> unplaced_neighbours;

	for (auto compartment : boost::make_iterator_range(neuron.adjacent_compartments(compartment))) {
		bool placed = false;
		for (auto placed_compartment : m_placed_compartments) {
			if (compartment == placed_compartment) {
				placed = true;
				break;
			}
		}
		if (!placed) {
			unplaced_neighbours.emplace(compartment);
		}
	}

	return unplaced_neighbours;
}

NumberTopBottom PlacementAlgorithmRuleset::place_simple(
    CoordinateSystem& coordinates,
    Neuron const& neuron,
    ResourceManager const& resources,
    size_t x_start,
    size_t y,
    CompartmentOnNeuron const& compartment,
    int direction,
    bool virtually)
{
	if (direction == 1) {
		return place_simple_right(
		    coordinates, neuron, resources, x_start, y, compartment, virtually);
	} else if (direction == -1) {
		return place_simple_left(
		    coordinates, neuron, resources, x_start, y, compartment, virtually);
	} else {
		throw std::invalid_argument("Invalid direction.");
	}
}

NumberTopBottom PlacementAlgorithmRuleset::place_simple_right(
    CoordinateSystem& coordinates,
    Neuron const& neuron,
    ResourceManager const& resources,
    size_t x_start,
    size_t y,
    CompartmentOnNeuron const& compartment,
    bool virtually)
{
	CoordinateSystem coordinates_copy = coordinates;

	LOG4CXX_TRACE(m_logger, "Virtual placement: " << virtually);
	LOG4CXX_DEBUG(m_logger, "Placing right: " << compartment << " at " << x_start << "," << y);

	NumberTopBottom required_resources = resources.get_config(compartment);

	// If not leaf increase size
	if (neuron.get_compartment_degree(compartment) > 1 && required_resources.number_total < 2) {
		required_resources.number_total = 2;
	}
	if (neuron.get_compartment_degree(compartment) > 2) {
		if (required_resources.number_total < 4) {
			required_resources.number_total = 4;
		}
		if (required_resources.number_bottom < 2) {
			required_resources.number_bottom = 2;
		}
		if (required_resources.number_top < 2) {
			required_resources.number_top = 2;
		}
	}

	LOG4CXX_TRACE(m_logger, "PlacementSize: " << required_resources);

	// Start at x_start and place to right in top row (only bottom if requested explicitly)
	for (size_t x = x_start; x < x_start + required_resources.number_top; x++) {
		if (coordinates_copy.coordinate_system[0][x].compartment) {
			throw std::runtime_error("Overlap During Placement at " + std::to_string(x) + ",0");
		}
		coordinates_copy.set_compartment(x, 0, compartment);
	}
	for (size_t x = x_start; x < x_start + required_resources.number_bottom; x++) {
		if (coordinates_copy.coordinate_system[1][x].compartment) {
			throw std::runtime_error("Overlap During Placement at " + std::to_string(x) + ",1");
		}
		coordinates_copy.set_compartment(x, 1, compartment);
	}
	if (required_resources.number_total >
	    required_resources.number_bottom + required_resources.number_top) {
		for (size_t x = x_start + required_resources.number_top;
		     x < x_start + required_resources.number_total - required_resources.number_bottom;
		     x++) {
			if (coordinates_copy.coordinate_system[y][x].compartment) {
				throw std::runtime_error(
				    "Overlap During Placement at " + std::to_string(x) + "," + std::to_string(y));
			}
			coordinates_copy.set_compartment(x, y, compartment);
		}
	}


	// Place internal Connections
	connect_self(coordinates_copy, compartment);

	if (!virtually) {
		m_placed_compartments.push_back(compartment);
		coordinates = coordinates_copy;
	}

	// Return placed neuron circuit nubmer
	return required_resources;
}

NumberTopBottom PlacementAlgorithmRuleset::place_simple_left(
    CoordinateSystem& coordinates,
    Neuron const& neuron,
    ResourceManager const& resources,
    size_t x_start,
    size_t y,
    CompartmentOnNeuron const& compartment,
    bool virtually)
{
	CoordinateSystem coordinates_copy = coordinates;

	LOG4CXX_TRACE(m_logger, "Virtual placement: " << virtually);
	LOG4CXX_DEBUG(m_logger, "Placing left: " << compartment << " at " << x_start << "," << y);

	NumberTopBottom required_resources = resources.get_config(compartment);

	// If not leaf increase size
	if (neuron.get_compartment_degree(compartment) > 1 && required_resources.number_total == 1) {
		required_resources.number_total = 2;
	}
	if (neuron.get_compartment_degree(compartment) > 2) {
		if (required_resources.number_total < 4) {
			required_resources.number_total = 4;
		}
		if (required_resources.number_bottom < 2) {
			required_resources.number_bottom = 2;
		}
		if (required_resources.number_top < 2) {
			required_resources.number_top = 2;
		}
	}

	LOG4CXX_TRACE(m_logger, "PlacementSize: " << required_resources);

	// Start at x_start and place to left in top row (only bottom if requested explicitly)
	for (size_t x = x_start; x > x_start - required_resources.number_top; x--) {
		if (coordinates_copy.coordinate_system[0][x].compartment) {
			throw std::runtime_error("Overlap During Placement at " + std::to_string(x) + ",0");
		}
		coordinates_copy.set_compartment(x, 0, compartment);
	}
	for (size_t x = x_start; x > x_start - required_resources.number_bottom; x--) {
		if (coordinates_copy.coordinate_system[1][x].compartment) {
			throw std::runtime_error("Overlap During Placement at " + std::to_string(x) + ",1");
		}
		coordinates_copy.set_compartment(x, 1, compartment);
	}
	if (required_resources.number_total >
	    required_resources.number_bottom + required_resources.number_top) {
		for (size_t x = x_start - required_resources.number_top;
		     x > x_start - required_resources.number_total + required_resources.number_bottom;
		     x--) {
			if (coordinates_copy.coordinate_system[y][x].compartment) {
				throw std::runtime_error(
				    "Overlap During Placement at " + std::to_string(x) + "," + std::to_string(y));
			}
			coordinates_copy.set_compartment(x, y, compartment);
		}
	}


	// Place internal Connections
	connect_self(coordinates_copy, compartment);

	if (!virtually) {
		m_placed_compartments.push_back(compartment);
		coordinates = coordinates_copy;
	}

	// Return placed neuron circuit number
	return required_resources;
}

NumberTopBottom PlacementAlgorithmRuleset::place_bridge(
    CoordinateSystem& coordinates,
    Neuron const& neuron,
    ResourceManager const& resources,
    size_t x_start,
    CompartmentOnNeuron const& compartment,
    int direction,
    bool virtually)
{
	if (direction == 1) {
		return place_bridge_right(coordinates, neuron, resources, x_start, compartment, virtually);
	} else if (direction == -1) {
		return place_bridge_left(coordinates, neuron, resources, x_start, compartment, virtually);
	} else {
		throw std::invalid_argument("Invalid direction.");
	}
}

NumberTopBottom PlacementAlgorithmRuleset::place_bridge_right(
    CoordinateSystem& coordinates,
    Neuron const& neuron,
    ResourceManager const& resources,
    size_t x_start,
    CompartmentOnNeuron const& compartment,
    bool virtually)
{
	CoordinateSystem coordinates_copy = coordinates;

	NumberTopBottom total_placed;
	// Throw exception if spot is already taken
	if (coordinates_copy.coordinate_system[0][x_start].compartment ||
	    coordinates_copy.coordinate_system[1][x_start].compartment) {
		throw std::logic_error("Cant place bridge: Space already taken.");
	}

	// Count allocated Neuron circuits (right and left limit)
	NumberTopBottom number_placed = NumberTopBottom(4, 2, 2);

	LOG4CXX_TRACE(m_logger, "Virtual placement: " << virtually);
	LOG4CXX_DEBUG(m_logger, "Placing Bridge: " << compartment);

	std::set<CompartmentOnNeuron> neighbours_unplaced = unplaced_neighbours(neuron, compartment);
	CompartmentNeighbours neighbours_classified =
	    neuron.classify_neighbours(compartment, neighbours_unplaced);

	// Limit to be changed???
	if (neighbours_classified.branches.size() > 2) {
		throw std::logic_error("Too many branches: No Placement possible.");
	}


	// Calculate chain lenghtes
	std::map<CompartmentOnNeuron, NumberTopBottom> chains_lengthes;
	std::set<CompartmentOnNeuron> marked_compartments;
	for (auto chain : neighbours_classified.chains) {
		chains_lengthes.emplace(chain, resources.get_config(chain));
	}
	if (chains_lengthes.size() != neighbours_classified.chains.size()) {
		throw std::logic_error("Not all chains measured for length.");
	}

	// Calculate Space required for placement of leafs
	NumberTopBottom leafs_size;
	for (auto leaf : neighbours_classified.leafs) {
		leafs_size += resources.get_config(leaf);
	}

	size_t x_temp = x_start;

	// Place bridge compartments at left limit
	coordinates_copy.coordinate_system[0][x_temp].compartment = compartment;
	coordinates_copy.coordinate_system[1][x_temp].compartment = compartment;

	// Place leafs inside
	total_placed += place_leafs(
	    coordinates_copy, neuron, resources, x_temp, 0, compartment, neighbours_classified.leafs, 1,
	    virtually);
	number_placed += leafs_size;

	x_temp += leafs_size.number_total - leafs_size.number_bottom;

	// Continue shared line of leaf placement
	coordinates_copy.coordinate_system[0][x_temp].switch_shared_right = true;

	x_temp++;

	// Place bridge compartment in top and bottom row
	coordinates_copy.coordinate_system[0][x_temp].compartment = compartment;
	coordinates_copy.coordinate_system[1][x_temp].compartment = compartment;
	// Connect Bridge compartment to bridge on other side of leafs via shared line
	coordinates_copy.coordinate_system[0][x_temp].switch_circuit_shared = true;

	// Select smallest chains without top bottom specification
	std::map<CompartmentOnNeuron, NumberTopBottom> chains_inside;
	while (chains_lengthes.size() + neighbours_classified.branches.size() > 4) {
		CompartmentOnNeuron compartment_min = chains_lengthes.begin()->first;
		// Only chains without specified resources in top row can be placed inside the bridge
		NumberTopBottom min_size = NumberTopBottom(256, 0, 256);
		// Finding smallest chain
		for (auto [compartment_temp, neuron_circuits] : chains_lengthes) {
			if (neuron_circuits < min_size) {
				min_size = neuron_circuits;
				compartment_min = compartment_temp;
			}
		}
		chains_inside.emplace(compartment_min, min_size);
		chains_lengthes.erase(compartment_min);
	}

	// Place inner chains in bottom row
	size_t x_lower_limit_chains = x_temp;
	for (auto [compartment_inside, _] : chains_inside) {
		coordinates_copy.coordinate_system[1][x_temp].compartment = compartment;
		x_temp++;
		number_placed += NumberTopBottom(1, 0, 1);
		// Place chain
		NumberTopBottom chain_resources = place_chain(
		    coordinates_copy, neuron, resources, x_temp, 1,
		    neuron.chain_compartments(compartment_inside, compartment), 1, virtually);

		x_temp += chain_resources.number_total;
		total_placed += chain_resources;
	}

	// Place bridge compartment in top row
	for (size_t x = x_lower_limit_chains; x < x_temp; x++) {
		coordinates_copy.coordinate_system[0][x].compartment = compartment;
		number_placed += NumberTopBottom(1, 1, 0);
	}

	// Place bridge compartments at right limit
	coordinates_copy.coordinate_system[0][x_temp].compartment = compartment;
	coordinates_copy.coordinate_system[1][x_temp].compartment = compartment;

	// Calculate difference if missing resources
	NumberTopBottom number_required = resources.get_config(compartment);
	NumberTopBottom difference;
	if (number_placed < number_required) {
		LOG4CXX_TRACE(m_logger, "Bridge: More Resources Required");
		difference.number_total = number_required.number_total - number_placed.number_total;
		if (number_required.number_bottom - number_placed.number_bottom > 0) {
			difference.number_bottom = number_required.number_bottom - number_placed.number_bottom;
		}
		if (number_required.number_top - number_placed.number_top > 0) {
			difference.number_top = number_required.number_top - number_placed.number_top;
		}
	}

	// Place additional compartments to the right in top and bottom row
	size_t distance = difference.number_total - difference.number_bottom;

	for (size_t x = x_temp; x < x_temp + distance; x++) {
		coordinates_copy.coordinate_system[0][x].compartment = compartment;
		coordinates_copy.coordinate_system[1][x].compartment = compartment;
	}
	number_placed += NumberTopBottom(2 * distance, distance, distance);


	// Connect Bridge internal
	connect_self(coordinates_copy, compartment);

	if (!virtually) {
		// Push Bridge Compartment in list of placed Compartments
		m_placed_compartments.push_back(compartment);
		coordinates = coordinates_copy;
	}

	total_placed += number_placed;

	return total_placed;
}

NumberTopBottom PlacementAlgorithmRuleset::place_bridge_left(
    CoordinateSystem& coordinates,
    Neuron const& neuron,
    ResourceManager const& resources,
    size_t x_start,
    CompartmentOnNeuron const& compartment,
    bool virtually)
{
	CoordinateSystem coordinates_copy = coordinates;

	NumberTopBottom total_placed;

	// Throw exception if spot is already taken
	if (coordinates_copy.coordinate_system[0][x_start].compartment ||
	    coordinates_copy.coordinate_system[1][x_start].compartment) {
		throw std::logic_error("Cant place bridge: Space already taken.");
	}

	// Count allocated Neuron circuits (right and left limit)
	NumberTopBottom number_placed = NumberTopBottom(4, 2, 2);

	LOG4CXX_DEBUG(m_logger, "Placing Bridge: " << compartment);

	std::set<CompartmentOnNeuron> neighbours_unplaced = unplaced_neighbours(neuron, compartment);
	CompartmentNeighbours neighbours_classified =
	    neuron.classify_neighbours(compartment, neighbours_unplaced);

	if (neighbours_classified.branches.size() > 2) {
		throw std::logic_error("To many branches: No Placement possible");
	}

	// Calculate chain lenghtes
	std::map<CompartmentOnNeuron, NumberTopBottom> chains_lengthes;
	std::set<CompartmentOnNeuron> marked_compartments;
	for (auto chain : neighbours_classified.chains) {
		chains_lengthes.emplace(chain, resources.get_config(chain));
	}
	if (chains_lengthes.size() != neighbours_classified.chains.size()) {
		throw std::logic_error("Not all chains measured for length.");
	}

	// Calculate Space required for placement of leafs
	NumberTopBottom leafs_size;
	for (auto leaf : neighbours_classified.leafs) {
		leafs_size += resources.get_config(leaf);
	}

	size_t x_temp = x_start;

	// Place bridge compartments at right limit
	coordinates_copy.coordinate_system[0][x_temp].compartment = compartment;
	coordinates_copy.coordinate_system[1][x_temp].compartment = compartment;

	// Place leafs inside
	total_placed += place_leafs(
	    coordinates_copy, neuron, resources, x_temp, 0, compartment, neighbours_classified.leafs,
	    -1, virtually);
	number_placed += leafs_size;

	x_temp -= leafs_size.number_total - leafs_size.number_bottom;

	// Continue shared line of leaf placement
	coordinates_copy.coordinate_system[0][x_temp - 1].switch_shared_right = true;

	x_temp--;

	// Place bridge compartment in top and bottom row
	coordinates_copy.coordinate_system[0][x_temp].compartment = compartment;
	coordinates_copy.coordinate_system[1][x_temp].compartment = compartment;
	// Connect Bridge compartment to bridge on other side of leafs via shared line
	coordinates_copy.coordinate_system[0][x_temp].switch_circuit_shared = true;

	// Select smallest chains without top bottom specification
	std::map<CompartmentOnNeuron, NumberTopBottom> chains_inside;
	while (chains_lengthes.size() + neighbours_classified.branches.size() > 4) {
		CompartmentOnNeuron compartment_min = chains_lengthes.begin()->first;
		// Only chains without specified resources in top row can be placed inside the bridge
		NumberTopBottom min_size = NumberTopBottom(256, 0, 256);
		// Finding smallest chain
		for (auto [compartment_temp, neuron_circuits] : chains_lengthes) {
			if (neuron_circuits < min_size) {
				min_size = neuron_circuits;
				compartment_min = compartment_temp;
			}
		}
		chains_inside.emplace(compartment_min, min_size);
		chains_lengthes.erase(compartment_min);
	}

	// Place inner chains in bottom row
	size_t x_upper_limit_chains = x_temp;
	for (auto [compartment_inside, _] : chains_inside) {
		coordinates_copy.coordinate_system[1][x_temp].compartment = compartment;
		x_temp--;
		number_placed += NumberTopBottom(1, 0, 1);
		// Place chain
		NumberTopBottom chain_resources = place_chain(
		    coordinates_copy, neuron, resources, x_temp, 1,
		    neuron.chain_compartments(compartment_inside, compartment), -1, virtually);
		x_temp -= chain_resources.number_total;
		total_placed += chain_resources;
	}

	// Place bridge compartment in top row
	for (size_t x = x_upper_limit_chains; x > x_temp; x--) {
		coordinates_copy.coordinate_system[0][x].compartment = compartment;
		number_placed += NumberTopBottom(1, 1, 0);
	}

	// Place bridge compartments at left limit
	coordinates_copy.coordinate_system[0][x_temp].compartment = compartment;
	coordinates_copy.coordinate_system[1][x_temp].compartment = compartment;

	// Calculate difference if missing resources
	NumberTopBottom number_required = resources.get_config(compartment);
	NumberTopBottom difference;
	if (number_placed < number_required) {
		LOG4CXX_TRACE(m_logger, "Bridge: More Resources Required");
		difference.number_total = number_required.number_total - number_placed.number_total;
		if (number_required.number_bottom - number_placed.number_bottom > 0) {
			difference.number_bottom = number_required.number_bottom - number_placed.number_bottom;
		}
		if (number_required.number_top - number_placed.number_top > 0) {
			difference.number_top = number_required.number_top - number_placed.number_top;
		}
	}

	// Place additional compartments to the right in top and bottom row
	size_t distance = difference.number_total - difference.number_bottom;

	for (size_t x = x_temp; x > x_temp - distance; x--) {
		coordinates_copy.coordinate_system[0][x].compartment = compartment;
		coordinates_copy.coordinate_system[1][x].compartment = compartment;
	}
	number_placed += NumberTopBottom(2 * distance, distance, distance);


	// Connect Bridge internal
	connect_self(coordinates_copy, compartment);

	if (!virtually) {
		// Push Bridge Compartment in list of placed Compartments
		m_placed_compartments.push_back(compartment);
		coordinates = coordinates_copy;
	}

	total_placed += number_placed;
	return total_placed;
}

NumberTopBottom PlacementAlgorithmRuleset::place_leafs(
    CoordinateSystem& coordinates,
    Neuron const& neuron,
    ResourceManager const& resources,
    size_t x_start,
    size_t y,
    CompartmentOnNeuron compartment,
    std::vector<CompartmentOnNeuron> const& leafs,
    int direction,
    bool virtually)
{
	LOG4CXX_TRACE(m_logger, "Virtual placement: " << virtually);
	LOG4CXX_DEBUG(m_logger, "Placing Leafs");

	CoordinateSystem coordinates_copy = coordinates;

	NumberTopBottom total_placed;
	// Connect directly to shared line from this compartment
	coordinates_copy.coordinate_system[y][x_start].switch_circuit_shared = true;

	size_t x_temp = x_start + direction;

	if (direction != 1 && direction != -1) {
		throw std::invalid_argument("Direction other than -1 or 1 is invalid.");
	}

	// Assign NCs to leafs and connect them to shared line via conductance
	for (auto leaf : leafs) {
		if (direction == 1) {
			total_placed +=
			    place_simple_right(coordinates_copy, neuron, resources, x_temp, y, leaf, virtually);
			coordinates_copy.coordinate_system[y][x_temp].switch_circuit_shared_conductance = true;
			x_temp +=
			    resources.get_config(leaf).number_total - resources.get_config(leaf).number_bottom;
		} else if (direction == -1) {
			total_placed +=
			    place_simple_left(coordinates_copy, neuron, resources, x_temp, y, leaf, virtually);
			coordinates_copy.coordinate_system[y][x_temp].switch_circuit_shared_conductance = true;
			x_temp -=
			    resources.get_config(leaf).number_total - resources.get_config(leaf).number_bottom;
		}

		if (!virtually) {
			// Emplace in connected compartments
			m_placed_connections.emplace(std::make_pair(compartment, leaf));
		}
	}

	if (virtually) {
		return total_placed;
	}

	// Connect shared line
	if (direction == 1) {
		for (size_t x = x_start; x < x_temp - 1; x++) {
			coordinates_copy.coordinate_system[y][x].switch_shared_right = true;
		}
	} else if (direction == -1) {
		for (size_t x = x_start - 1; x > x_temp; x--) {
			coordinates_copy.coordinate_system[y][x].switch_shared_right = true;
		}
	}

	coordinates = coordinates_copy;

	return total_placed;
}

NumberTopBottom PlacementAlgorithmRuleset::place_chain(
    CoordinateSystem& coordinates,
    Neuron const& neuron,
    ResourceManager const& resources,
    size_t x_start,
    size_t y,
    std::vector<CompartmentOnNeuron> const& chain,
    int direction,
    bool virtually)
{
	CoordinateSystem coordinates_copy = coordinates;

	LOG4CXX_DEBUG(m_logger, "Placing chain at: " << x_start << "," << y);
	size_t x_temp = x_start;

	NumberTopBottom resources_placed;

	for (auto compartment : chain) {
		if (direction == 1) {
			NumberTopBottom resources_compartment = place_simple_right(
			    coordinates_copy, neuron, resources, x_temp, y, compartment, virtually);
			if (y == 0) {
				x_temp += resources_compartment.number_total - resources_compartment.number_bottom;
			} else {
				x_temp += resources_compartment.number_total - resources_compartment.number_top;
			}
			// Add to placed resources
			resources_placed += resources_compartment;
		} else if (direction == -1) {
			NumberTopBottom resources_compartment = place_simple_left(
			    coordinates_copy, neuron, resources, x_temp, y, compartment, virtually);
			if (y == 0) {
				x_temp -= resources_compartment.number_total + resources_compartment.number_bottom;
			} else {
				x_temp -= resources_compartment.number_total + resources_compartment.number_top;
			}
			resources_placed += resources_compartment;
		}
	}

	if (!virtually) {
		coordinates = coordinates_copy;
	}
	// Return number of placed neuron circuits
	return resources_placed;
}

// Connect compartment with itself
void PlacementAlgorithmRuleset::connect_self(
    CoordinateSystem& coordinates, CompartmentOnNeuron const& compartment)
{
	std::vector<std::pair<int, int>> compartment_locations =
	    coordinates.find_neuron_circuits(compartment);
	for (auto compartment_coordinates : compartment_locations) {
		size_t X = compartment_coordinates.first;
		size_t Y = compartment_coordinates.second;
		// Connection right
		if (X < 256 && coordinates.get_compartment(X + 1, Y) == compartment) {
			coordinates.coordinate_system[Y][X].switch_right = true;
		}
		// Connection top bottom
		if (coordinates.get_compartment(X, 1 - Y) == compartment) {
			coordinates.coordinate_system[Y][X].switch_top_bottom = true;
		}
	}
}

void PlacementAlgorithmRuleset::connect_adjacent(
    CoordinateSystem& coordinates,
    Neuron const& neuron,
    CompartmentOnNeuron const& compartment_a,
    CompartmentOnNeuron const& compartment_b)
{
	LOG4CXX_DEBUG(m_logger, "Connecting adjacent.");

	// Lambda to return wether the connection between two points is blocked.
	auto occupied = [&coordinates](size_t x_a, size_t x_b, size_t y) -> bool {
		return (coordinates.coordinate_system[y][x_a].switch_circuit_shared_conductance ||
		        coordinates.coordinate_system[y][x_b].switch_circuit_shared_conductance) ||
		       (coordinates.coordinate_system[y][x_a].switch_circuit_shared &&
		        coordinates.coordinate_system[y][x_b].switch_circuit_shared);
	};

	// Chose Compartment with more connections as source for connection (no resistor)
	CompartmentOnNeuron compartment_source, compartment_target;
	if (neuron.get_compartment_degree(compartment_a) >
	    neuron.get_compartment_degree(compartment_b)) {
		compartment_source = compartment_a;
		compartment_target = compartment_b;
	} else {
		compartment_source = compartment_b;
		compartment_target = compartment_a;
	}

	CoordinateLimits limits_source = find_limits(coordinates, compartment_source);

	for (auto limit : limits_source.top) {
		if (coordinates.coordinate_system[0][limit.upper + 1].compartment == compartment_target) {
			if (occupied(limit.upper, limit.upper + 1, 0)) {
				continue;
			}
			// Switch connection direction if one already connected
			if (coordinates.coordinate_system[0][limit.upper + 1].switch_circuit_shared) {
				LOG4CXX_TRACE(m_logger, "Target already source. Swapping direction of connection.");
				coordinates.connect_shared(limit.upper + 1, limit.upper, 0);
				return;
			}
			coordinates.connect_shared(limit.upper, limit.upper + 1, 0);
			return;

		} else if (
		    coordinates.coordinate_system[0][limit.lower - 1].compartment == compartment_target) {
			if (occupied(limit.lower, limit.lower - 1, 0)) {
				continue;
			}
			// Switch connection direction if one already connected
			if (coordinates.coordinate_system[0][limit.lower - 1].switch_circuit_shared) {
				LOG4CXX_TRACE(m_logger, "Target already source. Swapping direction of connection.");
				coordinates.connect_shared(limit.lower - 1, limit.lower, 0);
				return;
			}
			coordinates.connect_shared(limit.lower, limit.lower - 1, 0);
			return;
		}
	}
	for (auto limit : limits_source.bottom) {
		if (coordinates.coordinate_system[1][limit.upper + 1].compartment == compartment_target) {
			if (occupied(limit.upper, limit.upper + 1, 1)) {
				continue;
			}
			// Switch connection direction if one already connected
			if (coordinates.coordinate_system[1][limit.upper + 1].switch_circuit_shared) {
				LOG4CXX_TRACE(m_logger, "Target already source. Swapping direction of connection.");
				coordinates.connect_shared(limit.upper + 1, limit.upper, 1);
				return;
			}
			coordinates.connect_shared(limit.upper, limit.upper + 1, 1);
			return;
		} else if (
		    coordinates.coordinate_system[1][limit.lower - 1].compartment == compartment_target) {
			if (occupied(limit.lower, limit.lower - 1, 1)) {
				continue;
			}
			// Switch connection direction if one already connected
			if (coordinates.coordinate_system[1][limit.lower - 1].switch_circuit_shared) {
				LOG4CXX_TRACE(m_logger, "Target already source. Swapping direction of connection.");
				coordinates.connect_shared(limit.lower - 1, limit.lower, 1);
				return;
			}
			coordinates.connect_shared(limit.lower, limit.lower - 1, 1);
			return;
		}
	}
	throw std::logic_error("No adjacent neuron circuits found to connect.");
}


void PlacementAlgorithmRuleset::connect_distant(
    CoordinateSystem& coordinates,
    Neuron const& neuron,
    CompartmentOnNeuron const& compartment_a,
    CompartmentOnNeuron const& compartment_b)
{
	LOG4CXX_DEBUG(m_logger, "Connecting distant compartments.");
	CoordinateLimits limits_a = find_limits(coordinates, compartment_a);
	CoordinateLimits limits_b = find_limits(coordinates, compartment_b);

	if ((limits_a.top.empty() && limits_b.bottom.empty()) ||
	    (limits_a.bottom.empty() && limits_b.top.empty())) {
		throw std::logic_error("No neuron circuits in same row. Can not connect.");
	}

	size_t distance = 256;
	size_t x_a = 0;
	size_t x_b = 0;
	size_t y = 0;
	bool blocked = false;
	bool valid = false;

	// Lambda to return wether the connection between two points is blocked.
	auto occupied = [&coordinates](size_t x_a, size_t x_b, size_t y) -> bool {
		return (coordinates.coordinate_system[y][x_a].switch_circuit_shared_conductance ||
		        coordinates.coordinate_system[y][x_b].switch_circuit_shared_conductance) ||
		       (coordinates.coordinate_system[y][x_a].switch_circuit_shared &&
		        coordinates.coordinate_system[y][x_b].switch_circuit_shared);
	};

	// Check for closest limits in top row
	for (auto limit_a : limits_a.top) {
		for (auto limit_b : limits_b.top) {
			blocked = false;
			if (limit_a.lower > limit_b.upper) {
				// Check if shared line is already used
				for (size_t x = limit_b.upper; x < limit_a.lower; x++) {
					if (coordinates.coordinate_system[0][x].switch_shared_right) {
						blocked = true;
					}
				}
				// Check if distance is minimal
				if (!blocked && ((limit_a.lower - limit_b.upper) < distance)) {
					distance = limit_a.lower - limit_b.upper;
					// Check if circuits are already used to connect
					if (!occupied(limit_a.lower, limit_b.upper, 0)) {
						x_a = limit_a.lower;
						x_b = limit_b.upper;
						y = 0;
						valid = true;
					}
				}
			} else if (limit_a.upper < limit_b.lower) {
				// Check if shared line is already used
				for (size_t x = limit_a.upper; x < limit_b.lower; x++) {
					if (coordinates.coordinate_system[0][x].switch_shared_right) {
						blocked = true;
					}
				}
				// Check if distance is minimal
				if (!blocked && ((limit_b.lower - limit_a.upper) < distance)) {
					distance = limit_b.lower - limit_a.upper;
					// Check if circuits are already used to connect
					if (!occupied(limit_a.upper, limit_b.lower, 0)) {
						x_a = limit_a.upper;
						x_b = limit_b.lower;
						y = 0;
						valid = true;
					}
				}
			}
		}
	}
	// Check for closest limits in bottom row
	for (auto limit_a : limits_a.bottom) {
		for (auto limit_b : limits_b.bottom) {
			blocked = false;
			if (limit_a.lower > limit_b.upper) {
				// Check if shared line is already used
				for (size_t x = limit_b.upper; x < limit_a.lower; x++) {
					if (coordinates.coordinate_system[1][x].switch_shared_right) {
						blocked = true;
					}
				}
				// Check if distance is minimal
				if (!blocked && ((limit_a.lower - limit_b.upper) < distance)) {
					distance = limit_a.lower - limit_b.upper;
					// Check if circuits are already used to connect
					if (!occupied(limit_a.lower, limit_b.upper, 1)) {
						x_a = limit_a.lower;
						x_b = limit_b.upper;
						y = 1;
						valid = true;
					}
				}
			} else if (limit_a.upper < limit_b.lower) {
				// Check if shared line is already used
				for (size_t x = limit_a.upper; x < limit_b.lower; x++) {
					if (coordinates.coordinate_system[1][x].switch_shared_right) {
						blocked = true;
					}
				}
				// Check if distance is minimal
				if (!blocked && ((limit_b.lower - limit_a.upper) < distance)) {
					distance = limit_b.lower - limit_a.upper;
					// Check if circuits are already used to connect
					if (!occupied(limit_a.upper, limit_b.lower, 1)) {
						x_a = limit_a.upper;
						x_b = limit_b.lower;
						y = 1;
						valid = true;
					}
				}
			}
		}
	}

	if (!valid) {
		throw std::logic_error("No distant connection possible.");
	}

	// Determine source and target compartment
	if (coordinates.coordinate_system[y][x_a].switch_circuit_shared) {
		coordinates.connect_shared(x_a, x_b, y);
	} else if (coordinates.coordinate_system[y][x_b].switch_circuit_shared) {
		coordinates.connect_shared(x_b, x_a, y);
	} else if (
	    neuron.get_compartment_degree(compartment_a) >
	    neuron.get_compartment_degree(compartment_b)) {
		coordinates.connect_shared(x_a, x_b, y);
	} else {
		coordinates.connect_shared(x_b, x_a, y);
	}
}


void PlacementAlgorithmRuleset::connect_placed(CoordinateSystem& coordinates, Neuron const& neuron)
{
	LOG4CXX_DEBUG(m_logger, "Connecting all newly placed compartments.");
	std::vector<CompartmentOnNeuron> placed_last_step;
	if (!m_results.empty()) {
		placed_last_step = m_results.back().placed_compartments;
	}

	std::set<CompartmentOnNeuron> placed;
	for (auto compartment : m_placed_compartments) {
		placed.emplace(compartment);
	}

	// Check which compartments are placed in this step
	std::vector<CompartmentOnNeuron> newly_placed;
	for (size_t i = 0; i < m_placed_compartments.size(); i++) {
		if (i >= placed_last_step.size()) {
			newly_placed.push_back(m_placed_compartments[i]);
		}
	}
	LOG4CXX_TRACE(m_logger, "Newly placed: ");
	for (auto compartment : newly_placed) {
		LOG4CXX_TRACE(m_logger, compartment);
	}


	for (auto placed_compartment : newly_placed) {
		for (auto adjacent :
		     boost::make_iterator_range(neuron.adjacent_compartments(placed_compartment))) {
			if (placed.contains(adjacent) &&
			    !m_placed_connections.contains(std::make_pair(placed_compartment, adjacent)) &&
			    !m_placed_connections.contains(std::make_pair(adjacent, placed_compartment))) {
				LOG4CXX_DEBUG(
				    m_logger, "Connecting: " << placed_compartment << " and " << adjacent);
				try {
					connect_adjacent(coordinates, neuron, placed_compartment, adjacent);
				} catch (std::logic_error&) {
					connect_distant(coordinates, neuron, placed_compartment, adjacent);
				}
				m_placed_connections.emplace(placed_compartment, adjacent);
			}
		}
	}
}

// Runs Placement Algorithm one step
AlgorithmResult PlacementAlgorithmRuleset::run_one_step(
    Neuron const& neuron, ResourceManager const& resources, size_t step)
{
	AlgorithmResult result;
	if (!m_results.empty()) {
		result = m_results.back();
	}

	LOG4CXX_DEBUG(
	    m_logger,
	    "\n\n______________________________Step: " << step << "______________________________\n\n");


	// Place Compartment with most Connections in center of coordinate System
	if (step == 0) {
		// Find largest compartment (By number of connections)
		CompartmentOnNeuron compartment_first;
		compartment_first = neuron.get_max_degree_compartment();

		LOG4CXX_DEBUG(m_logger, "Placing first: " << compartment_first);

		// Classify neighbours
		CompartmentNeighbours neighbours_classified = neuron.classify_neighbours(compartment_first);

		// Check if bridge structure is required
		if (neighbours_classified.leafs.size() > 1 || neighbours_classified.total() > 4) {
			place_bridge_right(result.coordinate_system, neuron, resources, 128, compartment_first);
		} else {
			place_simple_right(
			    result.coordinate_system, neuron, resources, 128, 0, compartment_first);
		}

		// Print where first compartment has been placed
		output_placed(result.coordinate_system, compartment_first);
	}
	// Every step apart from first one
	else {
		m_placed_compartments = m_results.back().placed_compartments;

		// Last placed compartment
		CompartmentOnNeuron last_compartment;
		// Compartment placed next
		CompartmentOnNeuron next_compartment;
		// Set of unplaced neighbours of last compartment
		std::set<CompartmentOnNeuron> neighbours_unplaced;

		// Iterate over placed compartments to find next compartment to place
		if (m_breadth_first_search) {
			for (std::vector<CompartmentOnNeuron>::reverse_iterator it =
			         m_placed_compartments.rbegin();
			     it != m_placed_compartments.rend(); it++) {
				last_compartment = *it;
				neighbours_unplaced = unplaced_neighbours(neuron, *it);

				// If unplaced neighbours take this compartment.
				if (neighbours_unplaced.size() > 0) {
					break;
				}
			}
		}
		// Else do depth-first-placement by default
		else {
			for (auto placed_compartment : m_placed_compartments) {
				last_compartment = placed_compartment;
				neighbours_unplaced = unplaced_neighbours(neuron, placed_compartment);

				if (neighbours_unplaced.size() > 0) {
					break;
				}
			}
		}
		LOG4CXX_DEBUG(
		    m_logger, "Placed compartment with unplaced neighbours: " << last_compartment);

		// Classify unplaced neighbours of last placed compartment
		CompartmentNeighbours neighbours_classified =
		    neuron.classify_neighbours(last_compartment, neighbours_unplaced);

		// Throw exception if multiple unplaced leafs exist
		if (neighbours_classified.leafs.size() > 1) {
			throw std::logic_error("Too many unplaced leafs at last placed compartment.");
		}

		// Selection of next compartment to be placed : Choice to be discussed!!
		// If available place whole chain directly and connect it.
		if (neighbours_classified.chains.size() > 0) {
			next_compartment = neighbours_classified.chains.front();
			std::vector<CompartmentOnNeuron> chain =
			    neuron.chain_compartments(next_compartment, last_compartment);

			CoordinateSystem coordinate_dummy;
			bool search_block = false;
			NumberTopBottom required_space =
			    place_chain(coordinate_dummy, neuron, resources, 128, 0, chain, 1, true);

			if (required_space.number_top != 0 && required_space.number_bottom != 0) {
				search_block = true;
			}


			PlacementSpot next_spot = select_free_spot(
			    find_free_spots(result.coordinate_system, last_compartment), required_space, true,
			    false, search_block);

			place_chain(
			    result.coordinate_system, neuron, resources, next_spot.x, next_spot.y, chain,
			    next_spot.direction);

			// Connect all placed compartments
			connect_placed(result.coordinate_system, neuron);

			// Save Placement Result for backtracking
			result.placed_compartments = m_placed_compartments;
			m_results.push_back(result);

			// Check if finished
			if (neuron.num_compartments() == m_placed_compartments.size()) {
				result.finished = true;
				LOG4CXX_DEBUG(m_logger, "Placement finished succesfully.");
			}

			return result;
		}
		// Else select branch
		else if (neighbours_classified.branches.size() > 0) {
			next_compartment = neighbours_classified.branches.front();
			LOG4CXX_DEBUG(m_logger, "Next Compartment to place (Branch): " << next_compartment);
		}
		// Else select leaf (Should only be one or none)
		else if (neighbours_classified.leafs.size() > 0) {
			next_compartment = neighbours_classified.leafs.front();
			LOG4CXX_DEBUG(m_logger, "Next Compartment to place (Leaf): " << next_compartment);

		}
		// Else throw error (no unplaced neighbours)
		else {
			throw std::logic_error(
			    "No leafs, chains or branches to place -> No unplaced neighbours.");
		}


		// Find adjacent structures of next compartment to be placed
		neighbours_unplaced = unplaced_neighbours(neuron, next_compartment);
		neighbours_classified = neuron.classify_neighbours(next_compartment, neighbours_unplaced);


		// Place bridge if required
		if (neighbours_classified.leafs.size() > 1 || neighbours_classified.total() > 4) {
			// Virtually place bridge to determine required space
			CoordinateSystem virtual_coordinates;
			NumberTopBottom required_space = place_bridge(
			    virtual_coordinates, neuron, resources, 128, next_compartment, 1, true);

			// Find placement spot with sufficient space
			PlacementSpot next_spot = select_free_spot(
			    find_free_spots(result.coordinate_system, last_compartment), required_space, true,
			    true, true);

			place_bridge(
			    result.coordinate_system, neuron, resources, next_spot.get_free_block_space().first,
			    next_compartment, next_spot.direction);
		}
		// Else simple placement
		else {
			// Virtually place simple to determine required space
			CoordinateSystem virtual_coordinates;
			NumberTopBottom required_space = place_simple(
			    virtual_coordinates, neuron, resources, 128, 0, next_compartment, 1, true);

			// Search for a placement spot that is a block if circuits in both rows required
			bool search_block = false;

			if (required_space.number_top != 0 && required_space.number_bottom != 0) {
				search_block = true;
			}


			// Find placement spot
			PlacementSpot next_spot = select_free_spot(
			    find_free_spots(result.coordinate_system, last_compartment), required_space, true,
			    false, search_block);

			// Choose normal spot or block spot depending on shape of compartment
			if (next_spot.y == 0 && required_space.number_bottom > 0) {
				place_simple(
				    result.coordinate_system, neuron, resources,
				    next_spot.get_free_block_space().first, next_spot.y, next_compartment,
				    next_spot.direction);
			} else if (next_spot.y == 1 && required_space.number_top > 0) {
				place_simple(
				    result.coordinate_system, neuron, resources,
				    next_spot.get_free_block_space().first, next_spot.y, next_compartment,
				    next_spot.direction);
			} else {
				place_simple(
				    result.coordinate_system, neuron, resources, next_spot.x, next_spot.y,
				    next_compartment, next_spot.direction);
			}
		}

		output_placed(result.coordinate_system, next_compartment);
	}

	// Connect all placed compartments
	connect_placed(result.coordinate_system, neuron);

	// Save Placement Result for backtracking
	result.placed_compartments = m_placed_compartments;
	m_results.push_back(result);

	// Check if finished
	if (neuron.num_compartments() == m_placed_compartments.size()) {
		result.finished = true;
		LOG4CXX_DEBUG(m_logger, "Placement finished succesfully.");
	}

	return result;
}

void PlacementAlgorithmRuleset::output_placed(
    CoordinateSystem const& coordinates, CompartmentOnNeuron const& compartment) const
{ // Console Output of compartment limits
	CoordinateLimits limits = find_limits(coordinates, compartment);
	LOG4CXX_DEBUG(m_logger, compartment << " placed in Limits: ");
	for (auto limit : limits.top) {
		LOG4CXX_DEBUG(m_logger, "Top:    " << limit.lower << " : " << limit.upper);
	}
	for (auto limit : limits.bottom) {
		LOG4CXX_DEBUG(m_logger, "Bottom: " << limit.lower << " : " << limit.upper);
	}
}

} // namespace grenade::vx::network::abstract