#include "grenade/vx/network/abstract/multicompartment/placement/algorithm_ruleset.h"
#include <sstream>
#include <log4cxx/logger.h>

namespace grenade::vx::network::abstract {

PlacementAlgorithmRuleset::PlacementAlgorithmRuleset() :
    m_logger(log4cxx::Logger::getLogger("grenade.MC.Placement.Ruleset"))
{
}

AlgorithmResult PlacementAlgorithmRuleset::run(
    CoordinateSystem const&, Neuron const& neuron, ResourceManager const& resources)
{
	if (!neuron.is_connected()) {
		throw std::runtime_error("Placement of not fully connected neuron is not supported.");
	}

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
	m_results.clear();
}

// Find right and left limit of compartment placed
CoordinateLimits PlacementAlgorithmRuleset::find_limits(
    CoordinateSystem const& coordinates, CompartmentOnNeuron const& compartment) const
{
	CoordinateLimits limits;
	// Iterate over Top and Bottom Row
	for (size_t y = 0; y < coordinates.coordinate_system.size(); y++) {
		bool finished = false;

		// Search for another limit until end of coordinate system is reached
		while (!finished) {
			CoordinateLimit limit{0, 0};
			bool valid_limit = false;

			// Top Row Check if previous Limits exist
			if (y == 0) {
				if (!limits.top.empty()) {
					limit.lower = limits.top.back().upper + 1;
				}
			}
			// Bottom Row Check if Limits exist
			else if (y == 1) {
				if (!limits.bottom.empty()) {
					limit.lower = limits.bottom.back().upper + 1;
				}
			}

			if (limit.lower >= coordinates.coordinate_system.at(y).size()) {
				finished = true;
				valid_limit = false;
				continue;
			}

			// Find next or first Lower Limit
			for (size_t x = limit.lower; x < coordinates.coordinate_system.at(y).size(); x++) {
				if (coordinates.coordinate_system.at(y).at(x).compartment == compartment) {
					limit.lower = x;
					valid_limit = true;
					break;
				}
				if (x == coordinates.coordinate_system.at(y).size() - 1) {
					finished = true;
					valid_limit = false;
				}
			}
			// Find Upper Limit to Lower Limit
			if (!finished && valid_limit) {
				for (size_t x = limit.lower; x < coordinates.coordinate_system.at(y).size(); x++) {
					if (x == coordinates.coordinate_system.at(y).size() - 1) {
						finished = true;
						limit.upper = x;
						break;
					}
					assert(x + 1 < coordinates.coordinate_system.at(y).size());
					if (coordinates.coordinate_system.at(y).at(x + 1).compartment != compartment) {
						limit.upper = x;
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
    CoordinateSystem const& coordinates,
    size_t x_start,
    size_t y,
    std::set<CompartmentOnNeuron> const& neighbours,
    int direction,
    bool search_block)
{
	LOG4CXX_DEBUG(
	    m_logger, "Checking for free spots at: [x= " << x_start << ",y= " << y
	                                                 << ",dir= " << direction << "]");
	std::vector<PlacementSpot> spots;

	PlacementSpot spot;
	spot.y = y;
	spot.x = x_start;
	spot.x_parent = x_start;
	spot.direction = direction;
	bool spot_found = false;


	if (coordinates.coordinate_system.at(y).at(x_start).switch_circuit_shared_conductance) {
		LOG4CXX_DEBUG(
		    m_logger, "Circuit is connected via the conductance to the shared line. "
		                  << "No free spots from this starting point");
		return spots;
	}

	if (!coordinates.connected_to_shared_line(x_start, y) &&
	    coordinates.coordinate_system.at(y).at(x_start).switch_shared_right) {
		// Shared line is already used to connect two compartments.
		// In case the shared line is used to connect another neuron circuit of
		// the parent compartment, this line will be found for this starting point
		// and we do not need to consider it here. Otherwise, the shared line can
		// not be reused for this connection.
		LOG4CXX_DEBUG(
		    m_logger, "Shared line is used for other connection. "
		                  << "Do not consider this starting point.");
		return spots;
	}

	// First, we find all free spots. In a second step, we add information about free
	// space in the opposite row. Finally, we make the spots block-like if requested.

	// Direction 1 (right)
	if (direction == 1) {
		// Check for free space in requested row
		for (size_t x = x_start + 1; x < coordinates.coordinate_system.at(y).size(); x++) {
			// we can no longer use the shared line, if another circuit which is not
			// a sibling is connected to the shared line.
			if (coordinates.connected_to_shared_line(x, y) &&
			    coordinates.get_compartment(x, y).has_value()) {
				if (std::find(
				        neighbours.begin(), neighbours.end(),
				        coordinates.get_compartment(x, y).value()) == neighbours.end()) {
					break;
				}
			}
			if (!coordinates.coordinate_system.at(y).at(x).compartment) {
				// First free spot in x direction is marked
				if (!spot_found) {
					spot.x = x;
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
				if (!coordinates.coordinate_system.at(1 - y).at(x).compartment) {
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
		for (int x = x_start - 1; x >= 0; x--) {
			// we can no longer use the shared line, if another circuit which is not
			// a sibling is connected to the shared line.
			if (coordinates.connected_to_shared_line(x, y) &&
			    coordinates.get_compartment(x, y).has_value()) {
				if (std::find(
				        neighbours.begin(), neighbours.end(),
				        coordinates.get_compartment(x, y).value()) == neighbours.end()) {
					break;
				}
			}
			if (!coordinates.coordinate_system.at(y).at(x).compartment) {
				// First free spot in x direction is marked
				if (!spot_found) {
					spot.x = x;
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
			} else if (x == 0) {
				break;
			}
		}
		// For all found spots determine number of resources in opposite row
		for (auto& spot : spots) {
			// Check for free space in opposit row
			size_t spot_lower_limit = spot.x - spot.free_space.number_total + 1;
			for (size_t x = spot.x; x + 1 > spot_lower_limit; x--) {
				if (!coordinates.coordinate_system.at(1 - y).at(x).compartment) {
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
		throw std::invalid_argument("Invalid direction.");
	}

	std::vector<PlacementSpot> final_spots;
	if (search_block) {
		for (auto spot : spots) {
			if (spot.x_opposite < 256) {
				// calculate how much space is lost by aligning the starting points in each row
				size_t lost_space =
				    std::max(spot.x, spot.x_opposite) - std::min(spot.x, spot.x_opposite);
				// align starting points
				spot.x = spot.x_opposite;
				// recalculate available space and make it block-like
				if (spot.y == 0) {
					spot.free_space.number_top = spot.free_space.number_top - lost_space;
				} else {
					spot.free_space.number_bottom = spot.free_space.number_bottom - lost_space;
				}
				size_t space_per_row =
				    std::min(spot.free_space.number_top, spot.free_space.number_bottom);
				spot.free_space = NumberTopBottom(2 * space_per_row, space_per_row, space_per_row);
				final_spots.push_back(spot);
			}
		}
	} else {
		final_spots = spots;
	}
	LOG4CXX_DEBUG(m_logger, "Found " << final_spots.size() << " spots.");
	return final_spots;
}

std::vector<PlacementSpot> PlacementAlgorithmRuleset::find_free_spots(
    CoordinateSystem const& coordinates,
    CompartmentOnNeuron const& compartment,
    std::set<CompartmentOnNeuron> const& neighbours,
    bool search_block)
{
	CoordinateLimits limits = find_limits(coordinates, compartment);
	std::vector<PlacementSpot> spots_limit;

	// All spots found
	std::vector<PlacementSpot> spots;

	// Search for free spot in top row
	for (auto limit : limits.top) {
		spots_limit = find_free_spots(coordinates, limit.upper, 0, neighbours, 1, search_block);
		spots.insert(spots.end(), spots_limit.begin(), spots_limit.end());

		spots_limit = find_free_spots(coordinates, limit.lower, 0, neighbours, -1, search_block);
		spots.insert(spots.end(), spots_limit.begin(), spots_limit.end());
	}
	// Search for free spot in bottom row
	for (auto limit : limits.bottom) {
		spots_limit = find_free_spots(coordinates, limit.upper, 1, neighbours, 1, search_block);
		spots.insert(spots.end(), spots_limit.begin(), spots_limit.end());

		spots_limit = find_free_spots(coordinates, limit.lower, 1, neighbours, -1, search_block);
		spots.insert(spots.end(), spots_limit.begin(), spots_limit.end());
	}

	return spots;
}

PlacementSpot PlacementAlgorithmRuleset::select_free_spot(
    std::vector<PlacementSpot> spots,
    NumberTopBottom const& min_spot_size,
    CoordinateSystem& coordinates,
    CompartmentOnNeuron const& parent_compartment)
{
	LOG4CXX_DEBUG(m_logger, "Selecting free spot");
	std::stringstream ss;
	ss << "Given spots:[";
	for (auto const& spot : spots) {
		ss << spot;
	}
	ss << "]";
	LOG4CXX_TRACE(m_logger, ss.str());

	std::vector<PlacementSpot> large_enough_spots;
	for (auto const& spot : spots) {
		if (min_spot_size <= spot.free_space) {
			large_enough_spots.push_back(spot);
		}
	}

	if (large_enough_spots.empty()) {
		throw std::runtime_error("No valid placement spot found.");
	}

	// Count parent's connections to the shared line, per row.
	// Preferring the busier row aims to block only one shared line if possible.
	std::array<size_t, 2> connections{0, 0};
	for (auto const& circuit_idx : coordinates.find_neuron_circuits(parent_compartment)) {
		if (coordinates.connected_to_shared_line(circuit_idx.first, circuit_idx.second)) {
			connections[circuit_idx.second]++;
		}
	}
	bool const prefer_row_0 = connections[0] >= connections[1];

	std::sort(
	    large_enough_spots.begin(), large_enough_spots.end(),
	    [&](PlacementSpot const& a, PlacementSpot const& b) {
		    if (a.y != b.y) {
			    return prefer_row_0 ? (a.y < b.y) : (a.y > b.y);
		    }
		    if (a.distance() != b.distance()) {
			    return a.distance() < b.distance();
		    }
		    return a.free_space > b.free_space;
	    });

	PlacementSpot const final_spot = large_enough_spots.front();
	LOG4CXX_TRACE(m_logger, "Selected spot: " << final_spot);
	return final_spot;
}

std::set<CompartmentOnNeuron> PlacementAlgorithmRuleset::unplaced_neighbours(
    Neuron const& neuron, CompartmentOnNeuron const& compartment) const
{
	std::set<CompartmentOnNeuron> unplaced_neighbours;

	for (auto compartment : neuron.adjacent_compartments(compartment)) {
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
	CoordinateSystem coordinates_copy;
	if (!virtually) {
		coordinates_copy = coordinates;
	}

	LOG4CXX_TRACE(m_logger, "Virtual placement: " << virtually);
	LOG4CXX_DEBUG(m_logger, "Placing right: " << compartment << " at " << x_start << "," << y);

	NumberTopBottom required_resources = resources.get_config(compartment);
	NumberTopBottom placed_resources;

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
	if (x_start + required_resources.number_top > coordinates_copy.coordinate_system.at(0).size()) {
		throw std::runtime_error("Placement does not fit on coordinate system.");
	}
	for (size_t x = x_start; x < x_start + required_resources.number_top; x++) {
		if (coordinates_copy.coordinate_system.at(0).at(x).compartment) {
			throw std::logic_error("Overlap During Placement at " + std::to_string(x) + ",0");
		}
		coordinates_copy.set_compartment(x, 0, compartment);
		placed_resources += NumberTopBottom(1, 1, 0);
	}

	if (x_start + required_resources.number_bottom >
	    coordinates_copy.coordinate_system.at(1).size()) {
		throw std::runtime_error("Placement does not fit on coordinate system.");
	}
	for (size_t x = x_start; x < x_start + required_resources.number_bottom; x++) {
		if (coordinates_copy.coordinate_system.at(1).at(x).compartment) {
			throw std::logic_error("Overlap During Placement at " + std::to_string(x) + ",1");
		}
		coordinates_copy.set_compartment(x, 1, compartment);
		placed_resources += NumberTopBottom(1, 0, 1);
	}
	if (required_resources.number_total >
	    required_resources.number_bottom + required_resources.number_top) {
		if (x_start + required_resources.number_total - required_resources.number_bottom >
		    coordinates_copy.coordinate_system.at(y).size()) {
			throw std::runtime_error("Placement does not fit on coordinate system.");
		}
		for (size_t x = x_start + required_resources.number_top;
		     x < x_start + required_resources.number_total - required_resources.number_bottom;
		     x++) {
			if (coordinates_copy.coordinate_system.at(y).at(x).compartment) {
				throw std::logic_error(
				    "Overlap During Placement at " + std::to_string(x) + "," + std::to_string(y));
			}
			coordinates_copy.set_compartment(x, y, compartment);
			placed_resources += NumberTopBottom(1, 1 - y, y);
		}
	} else if ((y == 0) and (required_resources.number_top == 0)) {
		// always place at least one circuit in the requested row
		if (coordinates_copy.coordinate_system.at(0).at(x_start).compartment) {
			throw std::logic_error("Overlap During Placement at " + std::to_string(x_start) + ",0");
		}
		coordinates_copy.set_compartment(x_start, 0, compartment);
		placed_resources += NumberTopBottom(1, 1, 0);
	} else if ((y == 1) and (required_resources.number_bottom == 0)) {
		// always place at least one circuit in the requested row
		if (coordinates_copy.coordinate_system.at(1).at(x_start).compartment) {
			throw std::logic_error("Overlap During Placement at " + std::to_string(x_start) + ",1");
		}
		coordinates_copy.set_compartment(x_start, 1, compartment);
		placed_resources += NumberTopBottom(1, 0, 1);
	}
	// Place internal Connections
	connect_self(coordinates_copy, compartment);

	if (!virtually) {
		m_placed_compartments.push_back(compartment);
		coordinates = coordinates_copy;
	}

	// Return placed neuron circuit nubmer
	return placed_resources;
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
	CoordinateSystem coordinates_copy;
	if (!virtually) {
		coordinates_copy = coordinates;
	}

	LOG4CXX_TRACE(m_logger, "Virtual placement: " << virtually);
	LOG4CXX_DEBUG(m_logger, "Placing left: " << compartment << " at " << x_start << "," << y);

	NumberTopBottom required_resources = resources.get_config(compartment);
	NumberTopBottom placed_resources;

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
	if (int(x_start) - int(required_resources.number_top) < 0) {
		throw std::runtime_error("Placement does not fit on coordinate system.");
	}
	for (size_t x = x_start; x > x_start - required_resources.number_top; x--) {
		if (coordinates_copy.coordinate_system.at(0).at(x).compartment) {
			throw std::logic_error("Overlap During Placement at " + std::to_string(x) + ",0");
		}
		coordinates_copy.set_compartment(x, 0, compartment);
		placed_resources += NumberTopBottom(1, 1, 0);
	}

	if (int(x_start) - int(required_resources.number_bottom) < 0) {
		throw std::runtime_error("Placement does not fit on coordinate system.");
	}
	for (size_t x = x_start; x > x_start - required_resources.number_bottom; x--) {
		if (coordinates_copy.coordinate_system.at(1).at(x).compartment) {
			throw std::logic_error("Overlap During Placement at " + std::to_string(x) + ",1");
		}
		coordinates_copy.set_compartment(x, 1, compartment);
		placed_resources += NumberTopBottom(1, 0, 1);
	}
	if (required_resources.number_total >
	    required_resources.number_bottom + required_resources.number_top) {
		if (int(x_start) - int(required_resources.number_total) +
		        int(required_resources.number_bottom) <
		    0) {
			throw std::runtime_error("Placement does not fit on coordinate system.");
		}
		for (size_t x = x_start - required_resources.number_top;
		     x > x_start - required_resources.number_total + required_resources.number_bottom;
		     x--) {
			if (coordinates_copy.coordinate_system.at(y).at(x).compartment) {
				throw std::logic_error(
				    "Overlap During Placement at " + std::to_string(x) + "," + std::to_string(y));
			}
			coordinates_copy.set_compartment(x, y, compartment);
			placed_resources += NumberTopBottom(1, 1 - y, y);
		}
	} else if ((y == 0) and (required_resources.number_top == 0)) {
		// always place at least one circuit in the requested row
		if (coordinates_copy.coordinate_system.at(0).at(x_start).compartment) {
			throw std::logic_error("Overlap During Placement at " + std::to_string(x_start) + ",0");
		}
		coordinates_copy.set_compartment(x_start, 0, compartment);
		placed_resources += NumberTopBottom(1, 1, 0);
	} else if ((y == 1) and (required_resources.number_bottom == 0)) {
		// always place at least one circuit in the requested row
		if (coordinates_copy.coordinate_system.at(1).at(x_start).compartment) {
			throw std::logic_error("Overlap During Placement at " + std::to_string(x_start) + ",1");
		}
		coordinates_copy.set_compartment(x_start, 1, compartment);
		placed_resources += NumberTopBottom(1, 0, 1);
	}

	// Place internal Connections
	connect_self(coordinates_copy, compartment);

	if (!virtually) {
		m_placed_compartments.push_back(compartment);
		coordinates = coordinates_copy;
	}

	// Return placed neuron circuit number
	return placed_resources;
}

NumberTopBottom PlacementAlgorithmRuleset::place_chain(
    CoordinateSystem& coordinates,
    Neuron const& neuron,
    ResourceManager const& resources,
    PlacementSpot const& spot,
    std::vector<CompartmentOnNeuron> const& chain,
    bool virtually)
{
	CoordinateSystem coordinates_copy;
	if (!virtually) {
		coordinates_copy = coordinates;
	};

	LOG4CXX_DEBUG(m_logger, "Placing chain at: " << spot.x << "," << spot.y);
	size_t x_parent = spot.x_parent;
	size_t x_temp = spot.x;

	NumberTopBottom resources_placed;

	for (auto compartment : chain) {
		if (spot.direction == 1) {
			NumberTopBottom resources_compartment = place_simple_right(
			    coordinates_copy, neuron, resources, x_temp, spot.y, compartment, virtually);
			coordinates_copy.connect_shared(x_parent, x_temp, spot.y);
			if (spot.y == 0) {
				x_parent = x_temp + resources_compartment.number_top - 1;
				x_temp += resources_compartment.number_top;
			} else {
				x_parent = x_temp + resources_compartment.number_bottom - 1;
				x_temp += resources_compartment.number_bottom;
			}
			// Add to placed resources
			resources_placed += resources_compartment;
		} else if (spot.direction == -1) {
			NumberTopBottom resources_compartment = place_simple_left(
			    coordinates_copy, neuron, resources, x_temp, spot.y, compartment, virtually);
			coordinates_copy.connect_shared(x_parent, x_temp, spot.y);
			if (spot.y == 0) {
				x_parent = x_temp - resources_compartment.number_top + 1;
				x_temp -= resources_compartment.number_top;
			} else {
				x_parent = x_temp - resources_compartment.number_bottom + 1;
				x_temp -= resources_compartment.number_bottom;
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

NumberTopBottom PlacementAlgorithmRuleset::place_branching_compartment(
    CoordinateSystem& coordinates,
    Neuron const& neuron,
    ResourceManager const& resources,
    CompartmentOnNeuron const& compartment,
    CompartmentOnNeuron const& parent)
{
	LOG4CXX_DEBUG(m_logger, "Placing Branch");

	NumberTopBottom total_resources;

	if (std::set<CompartmentOnNeuron>(m_placed_compartments.begin(), m_placed_compartments.end())
	        .contains(compartment)) {
		throw std::runtime_error("Comaprtment already placed.");
	}

	auto neighbours_unplaced = unplaced_neighbours(neuron, compartment);
	auto neighbours_classified = neuron.classify_neighbours(compartment, neighbours_unplaced);

	// TODO: this might select spots where the rest of the branch does not fit.
	CoordinateSystem dummy_coordinates;
	bool search_block = false;
	NumberTopBottom required_space = place_simple(
	    dummy_coordinates, neuron, resources,
	    size_t(dummy_coordinates.coordinate_system.at(0).size() / 2), 0, compartment, 1, true);

	if (required_space.number_top != 0 && required_space.number_bottom != 0) {
		search_block = true;
	}
	// All neighbours of the last placed compartment.
	std::set<CompartmentOnNeuron> neighbours;
	for (auto compartment : neuron.adjacent_compartments(parent)) {
		neighbours.emplace(compartment);
	}

	PlacementSpot next_spot = select_free_spot(
	    find_free_spots(coordinates, parent, neighbours, search_block), required_space, coordinates,
	    parent);

	total_resources += place_simple(
	    coordinates, neuron, resources, next_spot.x, next_spot.y, compartment, next_spot.direction);
	coordinates.connect_shared(next_spot.x_parent, next_spot.x, next_spot.y);
	output_placed(coordinates, compartment);

	return total_resources;
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
		if (X < coordinates.coordinate_system.at(Y).size() &&
		    coordinates.get_compartment(X + 1, Y) == compartment) {
			coordinates.coordinate_system.at(Y).at(X).switch_right = true;
		}
		// Connection top bottom
		if (coordinates.get_compartment(X, 1 - Y) == compartment) {
			coordinates.coordinate_system.at(Y).at(X).switch_top_bottom = true;
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

		place_simple_right(
		    result.coordinate_system, neuron, resources,
		    size_t(result.coordinate_system.coordinate_system.at(0).size() / 2), 0,
		    compartment_first);

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

		// Find next compartment to place
		for (std::vector<CompartmentOnNeuron>::reverse_iterator it = m_placed_compartments.rbegin();
		     it != m_placed_compartments.rend(); it++) {
			last_compartment = *it;
			neighbours_unplaced = unplaced_neighbours(neuron, *it);

			// If unplaced neighbours take this compartment.
			if (neighbours_unplaced.size() > 0) {
				break;
			}
		}

		if (neighbours_unplaced.empty()) {
			throw std::logic_error("No unplaced compartments left.");
		}

		LOG4CXX_DEBUG(
		    m_logger, "Placed compartment with unplaced neighbours: " << last_compartment);

		// Classify unplaced neighbours of last placed compartment
		CompartmentNeighbours neighbours_classified =
		    neuron.classify_neighbours(last_compartment, neighbours_unplaced);
		// All neighbours of the last placed compartment.
		std::set<CompartmentOnNeuron> neighbours;
		for (auto compartment : neuron.adjacent_compartments(last_compartment)) {
			neighbours.emplace(compartment);
		}

		// Selection of next compartment to be placed

		// If available select leaf.
		if (neighbours_classified.leafs.size() > 0) {
			next_compartment = neighbours_classified.leafs.front();
			LOG4CXX_DEBUG(m_logger, "Next Compartment to place (Leaf): " << next_compartment);

			// Virtually place leaf to determine required space.
			CoordinateSystem coordinate_dummy;
			bool search_block = false;
			NumberTopBottom required_space = place_simple(
			    coordinate_dummy, neuron, resources, size_t(coordinate_dummy[0].size() / 2), 0,
			    next_compartment, 1, true);

			if (required_space.number_top != 0 && required_space.number_bottom != 0) {
				search_block = true;
			}

			PlacementSpot next_spot = select_free_spot(
			    find_free_spots(
			        result.coordinate_system, last_compartment, neighbours, search_block),
			    required_space, result.coordinate_system, last_compartment);

			place_simple(
			    result.coordinate_system, neuron, resources, next_spot.x, next_spot.y,
			    next_compartment, next_spot.direction);
			result.coordinate_system.connect_shared(next_spot.x_parent, next_spot.x, next_spot.y);

		}
		// Else if available place whole chain directly and connect it.
		else if (neighbours_classified.chains.size() > 0) {
			next_compartment = neighbours_classified.chains.front();
			LOG4CXX_DEBUG(m_logger, "Next Compartment to place (Chain): " << next_compartment);
			std::vector<CompartmentOnNeuron> chain =
			    neuron.branch_compartments(next_compartment, last_compartment);
			// Virtually place chain to determine required space.
			bool search_block = false;
			PlacementSpot virt_spot;
			virt_spot.y = 0;
			virt_spot.x = result.coordinate_system.coordinate_system.at(0).size() / 2;
			virt_spot.x_parent = virt_spot.x - 1;
			virt_spot.direction = 1;
			NumberTopBottom required_space =
			    place_chain(result.coordinate_system, neuron, resources, virt_spot, chain, true);

			if (required_space.number_top != 0 && required_space.number_bottom != 0) {
				search_block = true;
			}

			PlacementSpot next_spot = select_free_spot(
			    find_free_spots(
			        result.coordinate_system, last_compartment, neighbours, search_block),
			    required_space, result.coordinate_system, last_compartment);

			place_chain(result.coordinate_system, neuron, resources, next_spot, chain);
		}
		// Else select branch
		else if (neighbours_classified.branches.size() > 0) {
			next_compartment = neighbours_classified.branches.front();

			// Select smallest available branch first.
			std::set<CompartmentOnNeuron> marked_compartments{last_compartment};
			size_t min_size = 2 * result.coordinate_system.coordinate_system.at(0).size() +
			                  1; // Does not fit on coordinate system.
			for (auto const& branch : neighbours_classified.branches) {
				marked_compartments = {last_compartment};
				auto branch_size = neuron.branch_size(branch, marked_compartments);
				if (branch_size < min_size) {
					next_compartment = branch;
					min_size = branch_size;
				}
			}

			LOG4CXX_DEBUG(m_logger, "Next Compartment to place (Branch): " << next_compartment);

			place_branching_compartment(
			    result.coordinate_system, neuron, resources, next_compartment, last_compartment);

		}
		// Else throw error (no unplaced neighbours)
		else {
			throw std::logic_error("No unplaced neighbors. The algorithm should have terminated.");
		}
	}

	// Save Placement Result for backtracking
	result.placed_compartments = m_placed_compartments;
	m_results.push_back(result);

	// Check that no compartment is acounted twice for in the placed compartments.
	assert(
	    m_placed_compartments.size() ==
	    std::set<CompartmentOnNeuron>(m_placed_compartments.begin(), m_placed_compartments.end())
	        .size());

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

std::vector<AlgorithmResult> PlacementAlgorithmRuleset::get_results() const
{
	return m_results;
}

} // namespace grenade::vx::network::abstract
