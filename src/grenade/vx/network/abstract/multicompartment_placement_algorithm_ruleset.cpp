#include "grenade/vx/network/abstract/multicompartment_placement_algorithm_ruleset.h"

namespace grenade::vx::network::abstract {

PlacementAlgorithmRuleset::PlacementAlgorithmRuleset() {}

AlgorithmResult PlacementAlgorithmRuleset::run(
    CoordinateSystem const& coordinate_system,
    Neuron const& neuron,
    ResourceManager const& resources)
{
	size_t step = 0;
	AlgorithmResult result;

	while (true) {
		result = run_one_step(coordinate_system, neuron, resources, step);
		if (result.finished) {
			break;
		}
		step++;
	}

	return result;
}

std::unique_ptr<PlacementAlgorithm> PlacementAlgorithmRuleset::clone() const
{
	return std::make_unique<PlacementAlgorithmRuleset>();
}

void PlacementAlgorithmRuleset::reset()
{
	placed_compartments.clear();
	m_results.clear();
	additional_resources.clear();
	additional_resources_compartments.clear();
}


// Find Compartment with largest number of connections
CompartmentOnNeuron PlacementAlgorithmRuleset::find_max_deg(Neuron const& neuron) const
{
	CompartmentOnNeuron ID_max_out_deg = *(neuron.compartment_iterators().first);
	for (auto i = neuron.compartment_iterators().first; i != neuron.compartment_iterators().second;
	     ++i) {
		if (neuron.out_degree(*i) > neuron.out_degree(ID_max_out_deg)) {
			ID_max_out_deg = *i;
		}
	}
	return ID_max_out_deg;
}

/*Alternative for above
CompartmentOnNeuron PlacementAlgorithmRuleset::find_max_deg(Neuron const& neuron) const
{
    return std::max_element(
        *(neuron.compartment_iterators().first), *(neuron.compartment_iterators().second),
        [neuron](CompartmentOnNeuron compartment_temp, CompartmentOnNeuron compartment_max) {
            return neuron.out_degree(compartment_temp) > neuron.out_degree(compartment_max);
        });
}
*/

// Find right and left limit of compartment placed
CoordinateLimits PlacementAlgorithmRuleset::find_limits(
    CoordinateSystem const& coordinate_system, CompartmentOnNeuron const& compartment) const
{
	CoordinateLimits result;
	// Iterate over Top and Bottom Row
	for (int y = 0; y < 2; y++) {
		CoordinateLimit limit;
		limit.lower = -1;
		limit.upper = -1;

		bool finished = false;
		// Search for another limit until end of coordinate system is reached
		while (!finished) {
			// Top Row Check if Limits exist
			if (y == 0) {
				if (!result.top.empty()) {
					limit.upper = result.top.back().upper;
				}
			}
			// Bottom Row Check if Limits exist
			else if (y == 1) {
				if (!result.bottom.empty()) {
					limit.upper = result.bottom.back().upper;
				}
			}

			// Find next or first Lower Limit
			for (int x = limit.upper + 1; x < 256; x++) {
				if (x == 255) {
					finished = true;
					limit.lower = 256;
				}
				if (coordinate_system.coordinate_system[y][x].compartment == compartment) {
					limit.lower = x;
					break;
				}
			}
			// Find Upper Limit to Lower Limit
			if (!finished) {
				for (int x = limit.lower; x < 256; x++) {
					if (x == 255) {
						finished = true;
						limit.upper = x;
					}
					if (coordinate_system.coordinate_system[y][x + 1].compartment != compartment) {
						limit.upper = x;
						break;
					}
				}
			} else {
				limit.upper = 0;
			}

			// Push Limit in correct Vector
			if (y == 0 && limit.lower != 256 && limit.upper != 0) {
				result.top.push_back(limit);
			} else if (y == 1 && limit.lower != 256 && limit.upper != 0) {
				result.bottom.push_back(limit);
			}
		}
	}


	if (result.top.empty()) {
		CoordinateLimit temp_limit;
		temp_limit.lower = -1;
		temp_limit.upper = -1;
		result.top.push_back(temp_limit);
	}
	if (result.bottom.empty()) {
		CoordinateLimit temp_limit;
		temp_limit.lower = -1;
		temp_limit.upper = -1;
		result.bottom.push_back(temp_limit);
	}

	return result;
}

// Iterate over a compartment chain
int PlacementAlgorithmRuleset::iterate_compartments_rec(
    Neuron const& neuron,
    ResourceManager const& resources,
    CompartmentOnNeuron const& target,
    std::set<CompartmentOnNeuron>& marked_compartments) const
{
	int neuron_circuit_counter = resources.get_config(target).number_total;

	// Mark Compartment as visited
	marked_compartments.emplace(target);

	for (auto it = neuron.adjacent_compartments(target).first;
	     it != neuron.adjacent_compartments(target).second; it++) {
		if (!marked_compartments.contains(*it)) {
			int count = iterate_compartments_rec(neuron, resources, *it, marked_compartments);
			if (count == -1) {
				std::cout << "Iterate_Compartments_Rec: On Branch: " << target << std::endl;
				return -1;
			}
			neuron_circuit_counter += count;
		}
	}
	// Is not chain but branch
	if (neuron.out_degree(target) > 2) {
		std::cout << "Iterate_Compartments_Rec: Branch found" << target << std::endl;
		return -1;
	}
	return neuron_circuit_counter;
}


// Add additional resources
void PlacementAlgorithmRuleset::add_additional_resources(
    CoordinateSystem const& coordinates, int x, int y)
{
	std::cout << "add_additional_resources for compartment: " << std::endl;
	if (additional_resources.find(coordinates.coordinate_system[y][x].compartment) !=
	    additional_resources.end()) {
		additional_resources[coordinates.coordinate_system[y][x].compartment].number_total++;
	} else {
		NumberTopBottom nums;
		nums.number_total = 1;
		additional_resources.emplace(coordinates.coordinate_system[y][x].compartment, nums);
	}
	if (y == 1) {
		additional_resources[coordinates.coordinate_system[y][x].compartment].number_bottom++;
	} else if (y == 0) {
		additional_resources[coordinates.coordinate_system[y][x].compartment].number_top++;
	}
	std::cout << "additional_resources_compartments.push_back: ";
	if (coordinates.coordinate_system[y][x].compartment) {
		std::cout << coordinates.coordinate_system[y][x].compartment.value() << std::endl;
	} else {
		std::cout << "None" << std::endl;
	}
	additional_resources_compartments.push_back(coordinates.coordinate_system[y][x].compartment);
}


// Place in top row if not expicitly requested bottom

void PlacementAlgorithmRuleset::place_simple_right(
    CoordinateSystem& coordinate_system,
    Neuron const& neuron,
    ResourceManager const& resources,
    int x_start,
    int y,
    CompartmentOnNeuron const& compartment)
{
	CompartmentOnNeuron temp_compartment = compartment;

	std::cout << std::endl
	          << "Placing right: " << temp_compartment << " at " << x_start << std::endl;

	int num_top = resources.get_config(temp_compartment).number_top;
	int num_bottom = resources.get_config(temp_compartment).number_bottom;
	int num_total = resources.get_config(temp_compartment).number_total;
	if (additional_resources.find(compartment) != additional_resources.end()) {
		num_total += additional_resources[compartment].number_total;
		num_top += additional_resources[compartment].number_top;
		num_bottom += additional_resources[compartment].number_bottom;
	}

	// If not leaf increase size
	if (neuron.out_degree(temp_compartment) > 1 && num_total < 2) {
		num_total = 2;
	}
	if (neuron.out_degree(temp_compartment) > 2) {
		if (num_total < 4) {
			num_total = 4;
		}
		if (num_bottom < 2) {
			num_bottom = 2;
		}
		if (num_top < 2) {
			num_top = 2;
		}
	}
	bool overlap = 0;
	// Start at x_start and place to right in top row (only bottom if requested explicitly)
	for (int j = x_start; j < x_start + num_top; j++) {
		if (coordinate_system.coordinate_system[0][j].compartment) {
			add_additional_resources(coordinate_system, x_start, 1);
			overlap = 1;
		}
		coordinate_system.set_compartment(j, 0, temp_compartment);
	}
	for (int j = x_start; j < x_start + num_bottom; j++) {
		if (coordinate_system.coordinate_system[1][j].compartment) {
			add_additional_resources(coordinate_system, x_start, 0);
			overlap = 1;
		}
		coordinate_system.set_compartment(j, 1, temp_compartment);
	}
	if (num_total > num_bottom + num_top) {
		for (int j = x_start + num_top; j < x_start + num_total - num_bottom; j++) {
			if (coordinate_system.coordinate_system[y][j].compartment) {
				add_additional_resources(coordinate_system, x_start, 1 - y);
				overlap = 1;
				std::cout << "Overlap total: " << j << " , " << y
				          << coordinate_system.coordinate_system[y][j].compartment.value()
				          << std::endl;
			}
			coordinate_system.set_compartment(j, y, temp_compartment);
		}
	}
	// Throw Exception if overlap occured during placement
	if (overlap) {
		throw std::invalid_argument("Overlap During Placement");
	}

	placed_compartments.push_back(temp_compartment);

	// Place internal Connections
	connect_self(coordinate_system, temp_compartment);
}


void PlacementAlgorithmRuleset::place_simple_left(
    CoordinateSystem& coordinate_system,
    Neuron const& neuron,
    ResourceManager const& resources,
    int x_start,
    int y,
    CompartmentOnNeuron const& compartment)
{
	CompartmentOnNeuron temp_compartment = compartment;

	std::cout << std::endl
	          << "Placing left: " << temp_compartment << " at " << x_start << std::endl;

	int num_top = resources.get_config(temp_compartment).number_top;
	int num_bottom = resources.get_config(temp_compartment).number_bottom;
	int num_total = resources.get_config(temp_compartment).number_total;
	if (additional_resources.find(compartment) != additional_resources.end()) {
		num_total += additional_resources[compartment].number_total;
		num_top += additional_resources[compartment].number_top;
		num_bottom += additional_resources[compartment].number_bottom;
	}

	// If not leaf increase size
	if (neuron.out_degree(temp_compartment) > 1 && num_total == 1) {
		num_total = 2;
	}
	if (neuron.out_degree(temp_compartment) > 2) {
		if (num_total < 4) {
			num_total = 4;
		}
		if (num_bottom < 2) {
			num_bottom = 2;
		}
		if (num_top < 2) {
			num_top = 2;
		}
	}

	bool overlap = 0;
	// Start at x_start and place to left in top row (only bottom if requested explicitly)
	for (int j = x_start; j > x_start - num_top; j--) {
		if (coordinate_system.coordinate_system[0][j].compartment) {
			add_additional_resources(coordinate_system, x_start, 1);
			overlap = 1;
		}
		coordinate_system.set_compartment(j, 0, temp_compartment);
	}
	for (int j = x_start; j > x_start - num_bottom; j--) {
		if (coordinate_system.coordinate_system[1][j].compartment) {
			add_additional_resources(coordinate_system, x_start, 0);
			overlap = 1;
		}
		coordinate_system.set_compartment(j, 1, temp_compartment);
	}
	if (num_total > num_bottom + num_top) {
		for (int j = x_start - num_top; j > x_start - num_total + num_bottom; j--) {
			if (coordinate_system.coordinate_system[y][j].compartment) {
				add_additional_resources(coordinate_system, x_start, 1 - y);
				overlap = 1;
			}
			coordinate_system.set_compartment(j, y, temp_compartment);
		}
	}

	// Throw Exception if overlap occured during placement
	if (overlap) {
		throw std::invalid_argument("Overlap During Placement");
	}

	placed_compartments.push_back(temp_compartment);

	// Place internal Connections
	connect_self(coordinate_system, temp_compartment);
}

void PlacementAlgorithmRuleset::place_bridge_right(
    CoordinateSystem& coordinate_system,
    Neuron const& neuron,
    ResourceManager const& resources,
    int x_start,
    CompartmentOnNeuron const& compartment)
{
	// Count allocated Neuron circuits (right and left limit)
	NumberTopBottom number_placed;
	number_placed.number_top = 2;
	number_placed.number_bottom = 2;
	number_placed.number_total = 4;

	std::cout << "Placing Bridge: " << compartment << std::endl;

	std::map<CompartmentOnNeuron, size_t> chains, branches;
	// Number of Branches from the compartment
	// Iterate over all Connections of Compartment
	std::set<CompartmentOnNeuron> marked_compartments;
	marked_compartments.emplace(compartment);
	for (auto it = neuron.adjacent_compartments(compartment).first;
	     it != neuron.adjacent_compartments(compartment).second; it++) {
		int counter = iterate_compartments_rec(neuron, resources, *it, marked_compartments);
		if (counter == -1) {
			branches.emplace(*it, 0);
			std::cout << "Emplacing Branch: " << *it << std::endl;
		} else {
			chains.emplace(*it, counter);
		}
	}
	for (auto i : branches) {
		std::cout << "Branches: " << i.first << std::endl;
	}
	for (auto i : chains) {
		std::cout << "Chains: " << i.first << std::endl;
	}

	if (branches.size() > 2) {
		throw std::logic_error("To many branches: Can not be Placed");
	}
	std::cout << "Bridge: num_branches: " << branches.size() << " num_chains: " << chains.size()
	          << std::endl;

	// Check for overlap
	if (coordinate_system.coordinate_system[0][x_start].compartment) {
		add_additional_resources(coordinate_system, x_start, 0);
		throw std::logic_error("Overlap during Bridge Placement");
	} else if (coordinate_system.coordinate_system[1][x_start].compartment) {
		add_additional_resources(coordinate_system, x_start, 0);
		throw std::logic_error("Overlap during Bridge Placement");
	}

	// Left Limit
	coordinate_system.coordinate_system[0][x_start].compartment = compartment;
	coordinate_system.coordinate_system[1][x_start].compartment = compartment;


	// Middle Structure
	std::map<CompartmentOnNeuron, size_t> chains_inside;

	// Select smallest chains
	while (chains.size() + branches.size() > 4) {
		CompartmentOnNeuron compartment_min;
		compartment_min = chains.begin()->first;
		size_t min_size = 256;
		for (auto [compartment_temp, neuron_circuits] : chains) {
			if (neuron_circuits < min_size) {
				min_size = neuron_circuits;
				compartment_min = compartment_temp;
				std::cout << "Compartment Inside" << compartment_min << " Size: " << neuron_circuits
				          << std::endl;
			}
		}
		chains_inside.emplace(compartment_min, min_size);
		chains.erase(compartment_min);
	}


	size_t counter_inside = 0;
	for (auto [compartment_temp, neuron_circuits] : chains_inside) {
		// Place bottom row compartment to allow for connection
		coordinate_system.coordinate_system[1][x_start + counter_inside].compartment = compartment;
		counter_inside++;
		number_placed.number_bottom++;
		number_placed.number_total++;
		// Place chain inside
		place_simple_right(
		    coordinate_system, neuron, resources, x_start + counter_inside, 1, compartment_temp);
		counter_inside += neuron_circuits;
	}

	// Place Top Row parralel to chains
	for (size_t i = x_start + 1; i < x_start + counter_inside; i++) {
		coordinate_system.coordinate_system[0][i].compartment = compartment;
		number_placed.number_top++;
		number_placed.number_total++;
	}

	// Right Limit
	coordinate_system.coordinate_system[0][x_start + counter_inside].compartment = compartment;
	coordinate_system.coordinate_system[1][x_start + counter_inside].compartment = compartment;

	// Check for resource requirements or extra resources request
	NumberTopBottom number_required = resources.get_config(compartment);
	size_t difference = 0;
	if (number_placed < number_required) {
		std::cout << "Bridge: More Resources Required" << std::endl;
		difference = number_required.number_total - number_placed.number_total;
		if (number_required.number_bottom - number_placed.number_bottom > difference) {
			difference = number_required.number_bottom - number_placed.number_bottom;
		}
		if (number_required.number_top - number_placed.number_top > difference) {
			difference = number_required.number_top - number_placed.number_top;
		}
	}
	// Extra resources requested
	if (additional_resources.find(compartment) != additional_resources.end()) {
		std::cout << "Bridge: Additional Resources Required" << std::endl;
		number_required = additional_resources[compartment];
		difference = number_required.number_total - number_placed.number_total;
		if (number_required.number_bottom - number_placed.number_bottom > difference) {
			difference = number_required.number_bottom - number_placed.number_bottom;
		}
		if (number_required.number_top - number_placed.number_top > difference) {
			difference = number_required.number_top - number_placed.number_top;
		}
	}
	// Place additional compartments to the right in top and bottom row
	for (size_t i = x_start + counter_inside + 1; i < x_start + counter_inside + 1 + difference;
	     i++) {
		coordinate_system.coordinate_system[0][i].compartment = compartment;
		coordinate_system.coordinate_system[1][i].compartment = compartment;
	}


	// Push Bridge Compartment in list of placed Compartments
	placed_compartments.push_back(compartment);

	// Connect Bridge internal
	connect_self(coordinate_system, compartment);
}

void PlacementAlgorithmRuleset::place_bridge_left(
    CoordinateSystem& coordinate_system,
    Neuron const& neuron,
    ResourceManager const& resources,
    int x_start,
    CompartmentOnNeuron const& compartment)
{
	// Count allocated Neuron circuits (right and left limit)
	NumberTopBottom number_placed;
	number_placed.number_top = 2;
	number_placed.number_bottom = 2;
	number_placed.number_total = 4;

	std::cout << "Placing Bridge: " << compartment << std::endl;

	std::map<CompartmentOnNeuron, size_t> chains, branches;
	// Number of Branches from the compartment
	// Iterate over all Connections of Compartment
	std::set<CompartmentOnNeuron> marked_compartments;
	marked_compartments.emplace(compartment);
	for (auto it = neuron.adjacent_compartments(compartment).first;
	     it != neuron.adjacent_compartments(compartment).second; it++) {
		int counter = iterate_compartments_rec(neuron, resources, *it, marked_compartments);
		if (counter == -1) {
			branches.emplace(*it, 0);
			std::cout << "Emplacing Branch: " << *it << std::endl;
		} else {
			chains.emplace(*it, counter);
		}
	}
	for (auto i : branches) {
		std::cout << "Branches: " << i.first << std::endl;
	}
	for (auto i : chains) {
		std::cout << "Chains: " << i.first << std::endl;
	}

	if (branches.size() > 2) {
		throw std::logic_error("To many branches: Can not be Placed");
	}
	std::cout << "Bridge: num_branches: " << branches.size() << " num_chains: " << chains.size()
	          << std::endl;

	// Check for overlap
	if (coordinate_system.coordinate_system[0][x_start].compartment) {
		add_additional_resources(coordinate_system, x_start, 0);
		throw std::logic_error("Overlap during Bridge Placement");
	} else if (coordinate_system.coordinate_system[1][x_start].compartment) {
		add_additional_resources(coordinate_system, x_start, 0);
		throw std::logic_error("Overlap during Bridge Placement");
	}

	// Right Limit
	coordinate_system.coordinate_system[0][x_start].compartment = compartment;
	coordinate_system.coordinate_system[1][x_start].compartment = compartment;


	// Middle Structure
	std::map<CompartmentOnNeuron, size_t> chains_inside;

	// Select smallest chains
	while (chains.size() + branches.size() > 4) {
		CompartmentOnNeuron compartment_min;
		compartment_min = chains.begin()->first;
		size_t min_size = 256;
		for (auto [compartment_temp, neuron_circuits] : chains) {
			if (neuron_circuits < min_size) {
				min_size = neuron_circuits;
				compartment_min = compartment_temp;
				std::cout << "Compartment Inside" << compartment_min << " Size: " << neuron_circuits
				          << std::endl;
			}
		}
		chains_inside.emplace(compartment_min, min_size);
		chains.erase(compartment_min);
	}


	size_t counter_inside = 0;
	for (auto [compartment_temp, neuron_circuits] : chains_inside) {
		// Place bottom row compartment to allow for connection
		coordinate_system.coordinate_system[1][x_start - counter_inside].compartment = compartment;
		counter_inside++;
		number_placed.number_bottom++;
		number_placed.number_total++;
		// Place chain inside
		place_simple_right(
		    coordinate_system, neuron, resources, x_start - counter_inside, 1, compartment_temp);
		counter_inside += neuron_circuits;
	}

	// Place Top Row parralel to chains
	for (size_t i = x_start - 1; i > x_start - counter_inside; i--) {
		coordinate_system.coordinate_system[0][i].compartment = compartment;
		number_placed.number_top++;
		number_placed.number_total++;
	}

	// Left Limit
	coordinate_system.coordinate_system[0][x_start - counter_inside].compartment = compartment;
	coordinate_system.coordinate_system[1][x_start - counter_inside].compartment = compartment;

	// Check for resource requirements or extra resources request
	NumberTopBottom number_required = resources.get_config(compartment);
	size_t difference = 0;
	if (number_placed < number_required) {
		std::cout << "Bridge: More Resources Required" << std::endl;
		difference = number_required.number_total - number_placed.number_total;
		if (number_required.number_bottom - number_placed.number_bottom > difference) {
			difference = number_required.number_bottom - number_placed.number_bottom;
		}
		if (number_required.number_top - number_placed.number_top > difference) {
			difference = number_required.number_top - number_placed.number_top;
		}
	}
	// Extra resources requested
	if (additional_resources.find(compartment) != additional_resources.end()) {
		std::cout << "Bridge: Additional Resources Required" << std::endl;
		number_required = additional_resources[compartment];
		difference = number_required.number_total - number_placed.number_total;
		if (number_required.number_bottom - number_placed.number_bottom > difference) {
			difference = number_required.number_bottom - number_placed.number_bottom;
		}
		if (number_required.number_top - number_placed.number_top > difference) {
			difference = number_required.number_top - number_placed.number_top;
		}
	}
	// Place additional compartments to the left in top and bottom row
	for (size_t i = x_start - counter_inside - 1; i > x_start - counter_inside - 1 - difference;
	     i--) {
		coordinate_system.coordinate_system[0][i].compartment = compartment;
		coordinate_system.coordinate_system[1][i].compartment = compartment;
	}


	// Push Bridge Compartment in list of placed Compartments
	placed_compartments.push_back(compartment);

	// Connect Bridge internal
	connect_self(coordinate_system, compartment);
}


// Connect compartment with itself
void PlacementAlgorithmRuleset::connect_self(
    CoordinateSystem& coordinate_system, CompartmentOnNeuron const& compartment)
{
	std::vector<std::pair<int, int>> compartment_locations =
	    coordinate_system.find_compartment(compartment);
	for (auto i : compartment_locations) {
		int X = i.first;
		int Y = i.second;
		// Connection right
		if (X < 256 && coordinate_system.get_compartment(X + 1, Y) == compartment) {
			coordinate_system.coordinate_system[Y][X].switch_right = true;
		}
		// Connection top bottom
		if (coordinate_system.get_compartment(X, 1 - Y) == compartment) {
			coordinate_system.coordinate_system[Y][X].switch_top_bottom = true;
		}
	}
}

// Connect to neighbour
void PlacementAlgorithmRuleset::connect_next(
    CoordinateSystem& coordinate_system,
    Neuron const& neuron,
    int y,
    CompartmentOnNeuron const& compartment_a,
    CompartmentOnNeuron const& compartment_b)
{
	// Chose Compartment with more connections as source for connection (no resistor)
	CompartmentOnNeuron temp_compartment_source, temp_compartment_target;
	if (neuron.out_degree(compartment_a) > neuron.out_degree(compartment_b)) {
		temp_compartment_source = compartment_a;
		temp_compartment_target = compartment_b;
	} else {
		temp_compartment_source = compartment_b;
		temp_compartment_target = compartment_a;
	}

	// Set x_limits depending on top or bottom
	int x_limit_a_lower, x_limit_a_upper;
	CoordinateLimits limits;
	limits = find_limits(coordinate_system, temp_compartment_source);

	// Iterate over all limits of a compartment in the reqired row
	std::vector<CoordinateLimit> limits_row;
	if (y == 0) {
		limits_row = limits.top;
	} else if (y == 1) {
		limits_row = limits.bottom;
	}

	bool connected = false;
	for (auto limit : limits_row) {
		x_limit_a_lower = limit.lower;
		x_limit_a_upper = limit.upper;
		if (x_limit_a_lower == -1 || x_limit_a_upper == -1) {
			continue;
		}
		// Check if compartments are next to each other and connect them
		// source left, target right
		if (coordinate_system.coordinate_system[y][x_limit_a_upper + 1].compartment ==
		    temp_compartment_target) {
			coordinate_system.coordinate_system[y][x_limit_a_upper].switch_circuit_shared = 1;
			coordinate_system.coordinate_system[y][x_limit_a_upper].switch_shared_right = 1;
			coordinate_system.coordinate_system[y][x_limit_a_upper + 1]
			    .switch_circuit_shared_conductance = 1;
			connected = true;
			std::cout << "Connected " << temp_compartment_source << " and "
			          << temp_compartment_target << " right at " << x_limit_a_upper << std::endl;
			break;
		}
		// source right, target left
		else if (
		    coordinate_system.coordinate_system[y][x_limit_a_lower - 1].compartment ==
		    temp_compartment_target) {
			coordinate_system.coordinate_system[y][x_limit_a_lower].switch_circuit_shared = 1;
			coordinate_system.coordinate_system[y][x_limit_a_lower - 1].switch_shared_right = 1;
			coordinate_system.coordinate_system[y][x_limit_a_lower - 1]
			    .switch_circuit_shared_conductance = 1;
			connected = true;
			std::cout << "Connected " << temp_compartment_source << " and "
			          << temp_compartment_target << " left at " << x_limit_a_lower << std::endl;
			break;
		}
	}

	// No Connection available
	if (!connected) {
		throw std::invalid_argument(
		    "Can not Connect: No possible direct Connection (not adjecent)");
	}
}

// Connect Leafs
AlgorithmResult PlacementAlgorithmRuleset::connect_leafs(
    CoordinateSystem& coordinate_system,
    Neuron const& neuron,
    CompartmentOnNeuron const& compartment_leaf,
    CompartmentOnNeuron const& compartment_node)
{
	AlgorithmResult result;
	result.coordinate_system = coordinate_system;
	CoordinateLimits limits;
	limits = find_limits(coordinate_system, compartment_leaf);
	int x_pos_top = limits.top.back().lower;
	int x_pos_bottom = limits.bottom.back().lower;
	// Top Right Connection
	for (auto i = neuron
	                  .adjacent_compartments(
	                      coordinate_system.coordinate_system[0][x_pos_top + 1].compartment.value())
	                  .first;
	     i != neuron
	              .adjacent_compartments(
	                  coordinate_system.coordinate_system[0][x_pos_top + 1].compartment.value())
	              .second;
	     i++) {
		if (*i == compartment_node) {
			result.coordinate_system.coordinate_system[0][x_pos_top]
			    .switch_circuit_shared_conductance = 1;
			result.coordinate_system.coordinate_system[0][x_pos_top].switch_shared_right = 1;
		}
	}
	// Top Left Connection
	for (auto i = neuron
	                  .adjacent_compartments(
	                      coordinate_system.coordinate_system[0][x_pos_top - 1].compartment.value())
	                  .first;
	     i != neuron
	              .adjacent_compartments(
	                  coordinate_system.coordinate_system[0][x_pos_top - 1].compartment.value())
	              .second;
	     i++) {
		if (*i == compartment_node) {
			result.coordinate_system.coordinate_system[0][x_pos_top]
			    .switch_circuit_shared_conductance = 1;
			result.coordinate_system.coordinate_system[0][x_pos_top - 1].switch_shared_right = 1;
		}
	}
	// Bottom Right Connection
	for (auto i =
	         neuron
	             .adjacent_compartments(
	                 coordinate_system.coordinate_system[1][x_pos_bottom + 1].compartment.value())
	             .first;
	     i != neuron
	              .adjacent_compartments(
	                  coordinate_system.coordinate_system[1][x_pos_bottom + 1].compartment.value())
	              .second;
	     i++) {
		if (*i == compartment_node) {
			result.coordinate_system.coordinate_system[0][x_pos_bottom]
			    .switch_circuit_shared_conductance = 1;
			result.coordinate_system.coordinate_system[0][x_pos_bottom].switch_shared_right = 1;
		}
	}
	// Bottom Left Connection
	for (auto i =
	         neuron
	             .adjacent_compartments(
	                 coordinate_system.coordinate_system[1][x_pos_bottom - 1].compartment.value())
	             .first;
	     i != neuron
	              .adjacent_compartments(
	                  coordinate_system.coordinate_system[1][x_pos_bottom - 1].compartment.value())
	              .second;
	     i++) {
		if (*i == compartment_node) {
			result.coordinate_system.coordinate_system[0][x_pos_bottom]
			    .switch_circuit_shared_conductance = 1;
			result.coordinate_system.coordinate_system[0][x_pos_bottom - 1].switch_shared_right = 1;
		}
	}
	return result;
}

// Connect All Compartments
void PlacementAlgorithmRuleset::connect_all(
    CoordinateSystem& coordinate_system, Neuron const& neuron)
{
	std::cout << "Connecting all Compartments" << std::endl;
	// Connect Placed Compartments
	for (auto i = neuron.compartment_connection_iterators().first;
	     i != neuron.compartment_connection_iterators().second; i++) {
		CompartmentOnNeuron source = neuron.source(*i);
		CompartmentOnNeuron target = neuron.target(*i);
		bool placed = 0;
		for (auto i : placed_compartments) {
			if (i == source) {
				for (auto j : placed_compartments) {
					if (j == target) {
						placed = 1;
						break;
					}
				}
				if (placed) {
					break;
				}
			}
		}
		if (placed) {
			// Check wether both compartments have circuit in top or in bottom row to
			// connect
			try {
				connect_next(coordinate_system, neuron, 0, source, target);
			} catch (std::invalid_argument&) {
				connect_next(coordinate_system, neuron, 1, source, target);
			}
		}
	}
}

// Runs Placement Algorithm one step
AlgorithmResult PlacementAlgorithmRuleset::run_one_step(
    CoordinateSystem const& coordinate_system,
    Neuron const& neuron,
    ResourceManager const& resources,
    size_t step)
{
	AlgorithmResult result;
	result.coordinate_system = coordinate_system;
	bool result_invalid = true;

	int c = 0;
	while (result_invalid) {
		result_invalid = false;

		std::cout << std::endl
		          << "______________________________Step: " << step
		          << "______________________________" << std::endl
		          << std::endl;

		if (m_results.size() == 0) {
			result = AlgorithmResult();
			step = 0;
		} else {
			result = m_results.back();
		}

		// Place Compartment with most Connections in Center of coordinate System
		if (step == 0) {
			// Find Largest Compartment (By number of connections)
			CompartmentOnNeuron compartment_first;
			compartment_first = find_max_deg(neuron);

			std::cout << "Placing first: " << compartment_first << std::endl;

			// Check if bridge structure is required
			if (neuron.out_degree(compartment_first) > 4) {
				try {
					place_bridge_right(
					    result.coordinate_system, neuron, resources, 128, compartment_first);
				} catch (std::logic_error&) {
					place_bridge_left(
					    result.coordinate_system, neuron, resources, 128, compartment_first);
				}
			} else {
				place_simple_right(
				    result.coordinate_system, neuron, resources, 128, 0, compartment_first);
			}

			// Console Output to return where Compartment was placed
			CoordinateLimits limits;
			limits = find_limits(result.coordinate_system, compartment_first);
			std::cout << placed_compartments.back()
			          << " Placed in Limits: " << limits.top.front().lower << ", "
			          << limits.top.back().upper << ", " << limits.bottom.front().lower << ", "
			          << limits.bottom.back().upper << ", " << std::endl;
		} else {
			placed_compartments = m_results.back().placed_compartments;
			CompartmentOnNeuron last_compartment;
			CompartmentOnNeuron temp_compartment;
			std::vector<CompartmentOnNeuron> adjacent_compartments;

			// Iterate backwards over placed compartments to find next compartment to place
			for (std::vector<CompartmentOnNeuron>::reverse_iterator it =
			         placed_compartments.rbegin();
			     it != placed_compartments.rend(); it++) {
				last_compartment = *it;
				std::cout << "Checking for last Compartment: " << last_compartment << std::endl;

				// Find Compartment Adjecent to last placed compartment and check if already placed
				for (auto i :
				     boost::make_iterator_range(neuron.adjacent_compartments(last_compartment))) {
					std::cout << "Checking adjacent Compartment: " << i << std::endl;
					bool found = 0;
					for (auto j : placed_compartments) {
						if (i == j) {
							found = 1;
							std::cout << "Placed" << std::endl;
							break;
						}
					}
					if (!found) {
						adjacent_compartments.push_back(i);
						std::cout << "Not Placed, added to Adjacent_List" << std::endl;
					}
				}
				if (adjacent_compartments.size() != 0) {
					break;
				}
			}
			// Find adjacent compartment with most connections and place first
			if (adjacent_compartments.size() == 0) {
				throw std::logic_error("No adjacent compartment to placed compartments found.");
			}
			temp_compartment = adjacent_compartments.front();
			for (auto i : adjacent_compartments) {
				std::cout << "Adjacent_List: " << i << std::endl;
				if (neuron.out_degree(i) > neuron.out_degree(temp_compartment)) {
					temp_compartment = i;
				}
			}

			// Find limits of last compartment
			CoordinateLimits limits;
			limits = find_limits(result.coordinate_system, last_compartment);

			// Try placement for each limit top
			int x_limit_lower_top, x_limit_upper_top, x_limit_lower_bottom, x_limit_upper_bottom;
			bool placed = false;

			// Frame to catch exception when overlapping occures during placement
			try {
				// Bridge Placement for more than 4 connections
				if (neuron.out_degree(temp_compartment) > 4) {
					x_limit_upper_top = limits.top.back().upper;
					try {
						place_bridge_right(
						    result.coordinate_system, neuron, resources, x_limit_upper_top + 1,
						    temp_compartment);
					} catch (std::logic_error&) {
						place_bridge_left(
						    result.coordinate_system, neuron, resources, x_limit_upper_top + 1,
						    temp_compartment);
					}
					placed = true;
				}
				// else classical Placement
				else {
					// Top Row Placement for each limit
					for (auto limit_top : limits.top) {
						x_limit_lower_top = limit_top.lower;
						x_limit_upper_top = limit_top.upper;
						if (x_limit_lower_top == -1 || x_limit_upper_top == -1) {
							continue;
						}
						std::cout << "Last Compartment LTL: " << limit_top.lower
						          << ", LTU: " << limit_top.upper << std::endl;

						// Try place Top Right
						if (!result.coordinate_system.coordinate_system[0][x_limit_upper_top + 1]
						         .compartment) {
							std::cout << std::endl
							          << "Placing next top right: " << temp_compartment
							          << std::endl;
							place_simple_right(
							    result.coordinate_system, neuron, resources, x_limit_upper_top + 1,
							    0, temp_compartment);
							placed = true;
							break;
						}
						// Try place Top Left
						else if (!result.coordinate_system
						              .coordinate_system[0][x_limit_lower_top - 1]
						              .compartment) {
							std::cout << std::endl
							          << "Placing next top left: " << temp_compartment << std::endl;
							place_simple_left(
							    result.coordinate_system, neuron, resources, x_limit_lower_top - 1,
							    0, temp_compartment);
							placed = true;
							break;
						}
					}

					// Bottom Row Placement for each limit bottom
					for (auto limit_bottom : limits.bottom) {
						x_limit_lower_bottom = limit_bottom.lower;
						x_limit_upper_bottom = limit_bottom.upper;
						if (x_limit_lower_bottom == -1 || x_limit_upper_bottom == -1) {
							continue;
						}
						std::cout << "Last Compartment LBL: " << limit_bottom.lower
						          << ", LBU: " << limit_bottom.upper << std::endl;

						// Try place Bottom Right
						if (!placed &&
						    !result.coordinate_system.coordinate_system[1][x_limit_upper_bottom + 1]
						         .compartment) {
							std::cout << std::endl
							          << "Placing next bottom right: " << temp_compartment
							          << std::endl;
							place_simple_right(
							    result.coordinate_system, neuron, resources,
							    x_limit_upper_bottom + 1, 1, temp_compartment);
							placed = true;
							break;
						}
						// Try place Bottom Left
						else if (
						    !placed &&
						    !result.coordinate_system.coordinate_system[1][x_limit_lower_bottom - 1]
						         .compartment) {
							std::cout << std::endl
							          << "Placing next bottom left: " << temp_compartment
							          << std::endl;
							place_simple_left(
							    result.coordinate_system, neuron, resources,
							    x_limit_lower_bottom - 1, 1, temp_compartment);
							placed = true;
							break;
						}
					}
				}

				// Console Output of compartment limits
				limits = find_limits(result.coordinate_system, temp_compartment);
				std::cout << placed_compartments.back()
				          << "Placed in Limits: " << limits.top.front().lower << ", "
				          << limits.top.back().upper << ", " << limits.bottom.front().lower << ", "
				          << limits.bottom.back().upper << ", " << std::endl;
			}

			// Handle Overlapping
			catch (std::invalid_argument&) {
				// Restart Loop
				result_invalid = true;
				std::cout << "Catch Overlap" << std::endl;
				std::cout << "additional_resources_compartments.size()= "
				          << additional_resources_compartments.size() << std::endl;
				std::cout << "additional_resources_compartments.back()";
				if (additional_resources_compartments.back()) {
					std::cout << *additional_resources_compartments.back();
				} else {
					std::cout << "None";
				}
				std::cout << additional_resources[additional_resources_compartments.back()]
				          << std::endl;
				bool remove = 0;
				for (std::vector<AlgorithmResult>::iterator it = m_results.begin();
				     it != m_results.end(); it++) {
					for (auto i : (*it).placed_compartments) {
						if (i == additional_resources_compartments.back()) {
							remove = 1;
							break;
						}
					}
					if (remove) {
						std::cout << "m_results.erase" << std::endl;
						m_results.erase(it, m_results.end());
						break;
					}
				}
				std::cout << "m_results.size()= " << m_results.size() << std::endl;
				if (additional_resources_compartments.size() > 2) {
					throw std::invalid_argument("TEST BREAK");
				}
			}

			// No free space to place
			if (!placed && !result_invalid) {
				std::cout << "NO FREE SPACE" << std::endl;
				throw std::invalid_argument("Can not place Compartment: No free space");
			}
			if (++c > 2) {
				throw std::invalid_argument("TEST BREAK");
			}
		}
	}

	// Connect all placed compartments
	connect_all(result.coordinate_system, neuron);

	// Save Placement Result for backtracking
	result.placed_compartments = placed_compartments;
	m_results.push_back(result);

	// Check if finished
	if (neuron.num_compartments() == placed_compartments.size()) {
		result.finished = true;
		std::cout << "FINISHED" << std::endl;
	}

	return result;
}


} // namespace grenade::vx::network::abstract