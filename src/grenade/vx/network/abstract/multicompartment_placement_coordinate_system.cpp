#include "grenade/vx/network/abstract/multicompartment_placement_coordinate_system.h"

namespace grenade::vx::network::abstract {

void CoordinateSystem::set(size_t x, size_t y, NeuronCircuit const& neuron_circuit)
{
	coordinate_system[y][x] = neuron_circuit;
}

NeuronCircuit CoordinateSystem::get(size_t x, size_t y) const
{
	return coordinate_system[y][x];
}


void CoordinateSystem::set_compartment(size_t x, size_t y, CompartmentOnNeuron const& compartment)
{
	coordinate_system[y][x].compartment = compartment;
}

void CoordinateSystem::set_config(
    size_t x, size_t y, UnplacedNeuronCircuit const& neuron_circuit_config_in)
{
	coordinate_system[y][x].switch_shared_right = neuron_circuit_config_in.switch_shared_right;
	coordinate_system[y][x].switch_right = neuron_circuit_config_in.switch_right;
	coordinate_system[y][x].switch_top_bottom = neuron_circuit_config_in.switch_top_bottom;
	coordinate_system[y][x].switch_circuit_shared = neuron_circuit_config_in.switch_circuit_shared;
	coordinate_system[y][x].switch_circuit_shared_conductance =
	    neuron_circuit_config_in.switch_circuit_shared_conductance;
	coordinate_system[y][x].get_status();
}

std::optional<CompartmentOnNeuron> CoordinateSystem::get_compartment(size_t x, size_t y) const
{
	return coordinate_system[y][x].compartment;
}

UnplacedNeuronCircuit CoordinateSystem::get_config(size_t x, size_t y) const
{
	UnplacedNeuronCircuit switch_config;
	switch_config.switch_shared_right = coordinate_system[y][x].switch_shared_right;
	switch_config.switch_right = coordinate_system[y][x].switch_right;
	switch_config.switch_top_bottom = coordinate_system[y][x].switch_top_bottom;
	switch_config.switch_circuit_shared = coordinate_system[y][x].switch_circuit_shared;
	switch_config.switch_circuit_shared_conductance =
	    coordinate_system[y][x].switch_circuit_shared_conductance;
	switch_config.get_status();

	return switch_config;
}


std::vector<std::pair<int, int>> CoordinateSystem::find_compartment(
    CompartmentOnNeuron const compartment) const
{
	std::vector<std::pair<int, int>> results;
	for (size_t i = 0; i < 2; i++) {
		for (size_t j = 0; j < coordinate_system[i].size(); j++) {
			const std::optional<CompartmentOnNeuron> temp_compartment_on_neuron =
			    coordinate_system[i][j].compartment;
			if (temp_compartment_on_neuron == compartment) {
				results.push_back(std::pair<int, int>(j, i));
			}
		}
	}
	return results;
}

bool CoordinateSystem::operator==(CoordinateSystem const& other) const
{
	return coordinate_system == other.coordinate_system;
}

bool CoordinateSystem::operator!=(CoordinateSystem const& other) const
{
	return !(*this == other);
}


// Checks for Connections at coordinates
bool CoordinateSystem::connected(size_t x, size_t y) const
{
	return (
	    connected_left_shared(x, y) || connected_right_shared(x, y) || connected_top_bottom(x, y) ||
	    connected_left(x, y) || connected_right(x, y) ||
	    (connected_shared_short(x, y).size() != 0) ||
	    (connected_shared_conductance(x, y).size() != 0));
}
bool CoordinateSystem::connected_right_shared(size_t x, size_t y) const
{
	if (x == coordinate_system[y].size() - 1) {
		return false;
	}
	return (
	    (coordinate_system[y][x].switch_circuit_shared &&
	     !coordinate_system[y][x].switch_circuit_shared_conductance &&
	     coordinate_system[y][x].switch_shared_right &&
	     !coordinate_system[y][x + 1].switch_circuit_shared &&
	     coordinate_system[y][x + 1].switch_circuit_shared_conductance) ||
	    (!coordinate_system[y][x].switch_circuit_shared &&
	     coordinate_system[y][x].switch_circuit_shared_conductance &&
	     coordinate_system[y][x].switch_shared_right &&
	     coordinate_system[y][x + 1].switch_circuit_shared &&
	     !coordinate_system[y][x + 1].switch_circuit_shared_conductance));
}
bool CoordinateSystem::connected_left_shared(size_t x, size_t y) const
{
	if (x == 0) {
		return false;
	}
	return (connected_right_shared(x - 1, y));
}
bool CoordinateSystem::connected_top_bottom(size_t x, size_t y) const
{
	return (
	    coordinate_system[y][x].switch_top_bottom && coordinate_system[1 - y][x].switch_top_bottom);
}
bool CoordinateSystem::connected_right(size_t x, size_t y) const
{
	if (x == coordinate_system[y].size() - 1) {
		return false;
	}
	return (coordinate_system[y][x].switch_right);
}
bool CoordinateSystem::connected_left(size_t x, size_t y) const
{
	if (x == 0) {
		return false;
	}
	return (coordinate_system[y][x - 1].switch_right);
}
bool CoordinateSystem::has_empty_connections(size_t x_max) const
{
	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			// Top Bottom Only connected from one side
			if ((coordinate_system[y][x].switch_top_bottom &&
			     !coordinate_system[1 - y][x].switch_top_bottom)
			    // Connected to shared Line but shared line not connected to left or right
			    || ((coordinate_system[y][x].switch_circuit_shared ||
			         coordinate_system[y][x].switch_circuit_shared_conductance) &&
			        !(coordinate_system[y][x].switch_shared_right ||
			          (x != 0 && coordinate_system[y][x - 1].switch_shared_right)))
			    // Shared line goes to empty
			    || (coordinate_system[y][x].switch_shared_right &&
			        // No connection on the right
			        (!(coordinate_system[y][x + 1].switch_circuit_shared ||
			           coordinate_system[y][x + 1].switch_circuit_shared_conductance ||
			           coordinate_system[y][x + 1].switch_shared_right) ||
			         // No connection on the left
			         !(coordinate_system[y][x].switch_circuit_shared ||
			           coordinate_system[y][x].switch_circuit_shared_conductance ||
			           ((x != 0) && coordinate_system[y][x - 1].switch_shared_right))))) {
				return true;
			}
		}
	}
	return false;
}
bool CoordinateSystem::has_double_connections(size_t x_max) const
{
	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			if (connected_right(x, y) && connected_right_shared(x, y)) {
				// std::cout << "Double Connection" << std::endl;
				return true;
			}
		}
	}
	return false;
}
void CoordinateSystem::clear_empty_connections(size_t x_max)
{
	// Deleting empty shared lines from right to left
	for (size_t x = x_max; x > 0; x--) {
		for (size_t y = 0; y < 2; y++) {
			// Shared line closed but not connected
			if ((coordinate_system[y][x].switch_shared_right &&
			     // No connection on the right
			     (!(coordinate_system[y][x + 1].switch_circuit_shared ||
			        coordinate_system[y][x + 1].switch_circuit_shared_conductance ||
			        coordinate_system[y][x + 1].switch_shared_right) ||
			      // No connection on the left
			      !(coordinate_system[y][x].switch_circuit_shared ||
			        coordinate_system[y][x].switch_circuit_shared_conductance ||
			        ((x != 0) && coordinate_system[y][x - 1].switch_shared_right))))) {
				// Clear shared line to left until connection
				coordinate_system[y][x].switch_shared_right = false;
			}
		}
	}

	// Deleting empty shared lines from left to right
	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			// Shared line closed but not connected
			if ((coordinate_system[y][x].switch_shared_right &&
			     // No connection on the right
			     (!(coordinate_system[y][x + 1].switch_circuit_shared ||
			        coordinate_system[y][x + 1].switch_circuit_shared_conductance ||
			        coordinate_system[y][x + 1].switch_shared_right) ||
			      // No connection on the left
			      !(coordinate_system[y][x].switch_circuit_shared ||
			        coordinate_system[y][x].switch_circuit_shared_conductance ||
			        ((x != 0) && coordinate_system[y][x - 1].switch_shared_right))))) {
				// Clear shared line to left until connection
				coordinate_system[y][x].switch_shared_right = false;
			}
		}
	}

	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			// One Circuit connects to other row but circuit in other row does not connect
			if (coordinate_system[y][x].switch_top_bottom &&
			    !coordinate_system[1 - y][x].switch_top_bottom) {
				coordinate_system[y][x].switch_top_bottom = false;
			}
			// Connected to shared Line but Shared Line not connected to left or right
			if ((coordinate_system[y][x].switch_circuit_shared ||
			     coordinate_system[y][x].switch_circuit_shared_conductance) &&
			    !(coordinate_system[y][x].switch_shared_right ||
			      (x != 0 && coordinate_system[y][x - 1].switch_shared_right))) {
				coordinate_system[y][x].switch_circuit_shared = false;
				coordinate_system[y][x].switch_circuit_shared_conductance = false;
			}
		}
	}
}

void CoordinateSystem::clear_invalid_connections(size_t x_max)
{
	clear_empty_connections(x_max);
	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			// Connected via double conductance
			if (coordinate_system[y][x].switch_circuit_shared_conductance &&
			    coordinate_system[y][x].switch_shared_right &&
			    coordinate_system[y][x + 1].switch_circuit_shared_conductance) {
				coordinate_system[y][x].switch_circuit_shared_conductance = false;
				coordinate_system[y][x].switch_shared_right = false;
				coordinate_system[y][x + 1].switch_circuit_shared_conductance = false;
			}
			// Double connection
			if (coordinate_system[y][x].switch_circuit_shared_conductance &&
			    coordinate_system[y][x].switch_circuit_shared) {
				coordinate_system[y][x].switch_circuit_shared_conductance = false;
				coordinate_system[y][x].switch_circuit_shared = false;
			}
		}
	}
}

bool CoordinateSystem::short_circuit(size_t x_max) const
{
	for (size_t y = 0; y < 2; y++) {
		for (size_t x = 0; x < x_max; x++) {
			if (coordinate_system[y][x].switch_circuit_shared) {
				CompartmentOnNeuron temp_compartment = coordinate_system[y][x].compartment.value();
				size_t x_temp = x;
				while (coordinate_system[y][x_temp].switch_shared_right) {
					if (coordinate_system[y][x_temp].switch_circuit_shared &&
					    coordinate_system[y][x_temp].compartment.value() != temp_compartment) {
						return true;
					}
					x_temp++;
				}
			}
		}
	}
	return false;
}

std::vector<std::pair<size_t, size_t>> CoordinateSystem::connected_shared_short(
    size_t x, size_t y) const
{
	std::vector<std::pair<size_t, size_t>> connected_circuits;
	if (!coordinate_system[y][x].switch_circuit_shared) {
		return connected_circuits;
	}

	// Check for connected compartments on the right
	size_t x_temp = x;
	while (x_temp < coordinate_system[y].size() - 1 &&
	       coordinate_system[y][x_temp].switch_shared_right) {
		if (coordinate_system[y][x_temp + 1].switch_circuit_shared) {
			connected_circuits.push_back(std::make_pair(x_temp + 1, y));
		}
		x_temp++;
	}
	// Check for connected compartments on the left
	x_temp = x;
	while (x_temp > 0 && coordinate_system[y][x_temp - 1].switch_shared_right) {
		if (coordinate_system[y][x_temp - 1].switch_circuit_shared) {
			connected_circuits.push_back(std::make_pair(x_temp - 1, y));
		}
		x_temp--;
	}

	return connected_circuits;
}

std::vector<std::pair<size_t, size_t>> CoordinateSystem::connected_shared_conductance(
    size_t x, size_t y) const
{
	std::vector<std::pair<size_t, size_t>> connected_circuits;
	if (!coordinate_system[y][x].switch_circuit_shared) {
		return connected_circuits;
	}

	// Check for connected compartments on the right. This compartment needs to be connected
	// directly to the shared line and the other compartment needs to be connected via conductance.
	size_t x_temp = x;
	while (x_temp < coordinate_system[y].size() - 1 &&
	       coordinate_system[y][x_temp].switch_shared_right) {
		if (coordinate_system[y][x_temp + 1].switch_circuit_shared_conductance) {
			connected_circuits.push_back(std::make_pair(x_temp + 1, y));
		}
		x_temp++;
	}

	// Check for connected compartments on the left. This compartment needs to be connected
	// directly to the shared line and the other compartment needs to be connected via conductance.
	if (x == 0) {
		return connected_circuits;
	}
	x_temp = x - 1;
	while (x_temp > 0 && coordinate_system[y][x_temp].switch_shared_right) {
		if (coordinate_system[y][x_temp].switch_circuit_shared_conductance) {
			connected_circuits.push_back(std::make_pair(x_temp, y));
		}
		x_temp--;
	}
	return connected_circuits;
}

NumberTopBottom CoordinateSystem::assign_compartment_adjacent(
    size_t x, size_t y, CompartmentOnNeuron compartment)
{
	if (coordinate_system[y][x].compartment != CompartmentOnNeuron()) {
		return NumberTopBottom();
	}

	coordinate_system[y][x].compartment = compartment;
	NumberTopBottom number_neuron_circuits = NumberTopBottom(1, 1 - y, y);

	if (connected_right(x, y)) {
		number_neuron_circuits += assign_compartment_adjacent(x + 1, y, compartment);
	}
	if (connected_left(x, y)) {
		number_neuron_circuits += assign_compartment_adjacent(x - 1, y, compartment);
	}
	if (connected_top_bottom(x, y)) {
		number_neuron_circuits += assign_compartment_adjacent(x, 1 - y, compartment);
	}

	return number_neuron_circuits;
}

void CoordinateSystem::clear()
{
	for (size_t y = 0; y < 2; y++) {
		for (size_t x = 0; x < coordinate_system[y].size(); x++) {
			coordinate_system[y][x] = NeuronCircuit();
		}
	}
}


std::array<NeuronCircuit, 256>& CoordinateSystem::operator[](size_t y)
{
	return coordinate_system[y];
}


} // namespace grenade::vx::network::abstract