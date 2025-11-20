#include "grenade/vx/network/abstract/multicompartment_placement_coordinate_system.h"

namespace grenade::vx::network::abstract {

void CoordinateSystem::set(int x, int y, NeuronCircuit const& neuron_circuit)
{
	coordinate_system[y][x] = neuron_circuit;
}

NeuronCircuit CoordinateSystem::get(int x, int y) const
{
	return coordinate_system[y][x];
}


void CoordinateSystem::set_compartment(int x, int y, CompartmentOnNeuron const& compartment)
{
	coordinate_system[y][x].compartment = compartment;
}

void CoordinateSystem::set_config(
    int x, int y, UnplacedNeuronCircuit const& neuron_circuit_config_in)
{
	coordinate_system[y][x].switch_shared_right = neuron_circuit_config_in.switch_shared_right;
	coordinate_system[y][x].switch_right = neuron_circuit_config_in.switch_right;
	coordinate_system[y][x].switch_top_bottom = neuron_circuit_config_in.switch_top_bottom;
	coordinate_system[y][x].switch_circuit_shared = neuron_circuit_config_in.switch_circuit_shared;
	coordinate_system[y][x].switch_circuit_shared_conductance =
	    neuron_circuit_config_in.switch_circuit_shared_conductance;
	coordinate_system[y][x].get_status();
}

std::optional<CompartmentOnNeuron> CoordinateSystem::get_compartment(int x, int y) const
{
	return coordinate_system[y][x].compartment;
}

UnplacedNeuronCircuit CoordinateSystem::get_config(int x, int y) const
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
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 256; j++) {
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
	    connected_left(x, y) || connected_right(x, y));
}
bool CoordinateSystem::connected_left_shared(size_t x, size_t y) const
{
	if (x == 0) {
		return false;
	}
	return (
	    (coordinate_system[y][x - 1].switch_circuit_shared &&
	     coordinate_system[y][x - 1].switch_shared_right &&
	     coordinate_system[y][x].switch_circuit_shared_conductance) ||
	    (coordinate_system[y][x].switch_circuit_shared &&
	     coordinate_system[y][x - 1].switch_shared_right &&
	     coordinate_system[y][x - 1].switch_circuit_shared_conductance));
}
bool CoordinateSystem::connected_right_shared(size_t x, size_t y) const
{
	if (x == 255) {
		return false;
	}
	return (
	    (coordinate_system[y][x].switch_circuit_shared &&
	     coordinate_system[y][x].switch_shared_right &&
	     coordinate_system[y][x + 1].switch_circuit_shared_conductance) ||
	    (coordinate_system[y][x + 1].switch_circuit_shared &&
	     coordinate_system[y][x].switch_shared_right &&
	     coordinate_system[y][x].switch_circuit_shared_conductance));
}
bool CoordinateSystem::connected_top_bottom(size_t x, size_t y) const
{
	return (
	    coordinate_system[y][x].switch_top_bottom && coordinate_system[1 - y][x].switch_top_bottom);
}
bool CoordinateSystem::connected_right(size_t x, size_t y) const
{
	if (x == 255) {
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
	// std::cout << "Checking for empty Connections up to: " << x_max << std::endl;
	// std::cout << std::endl;
	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			// Top Bottom Only connected from one side
			if ((coordinate_system[y][x].switch_top_bottom &&
			     !coordinate_system[1 - y][x].switch_top_bottom)
			    // Connected to shared Line but Shared Line not connected to left or right
			    || ((coordinate_system[y][x].switch_circuit_shared ||
			         coordinate_system[y][x].switch_circuit_shared_conductance) &&
			        !(coordinate_system[y][x].switch_shared_right ||
			          (x != 0 && coordinate_system[y][x - 1].switch_shared_right)))

			    // Connected to shared line directly and shared line connected right but not
			    // with other compartment
			    || (coordinate_system[y][x].switch_circuit_shared &&
			        coordinate_system[y][x].switch_shared_right && x != 255 &&
			        !coordinate_system[y][x + 1].switch_circuit_shared_conductance)
			    // Connected to shared line directly and shared line connected left but not
			    // with other compartment
			    || (coordinate_system[y][x].switch_circuit_shared && x != 0 &&
			        coordinate_system[y][x - 1].switch_shared_right &&
			        !coordinate_system[y][x - 1].switch_circuit_shared_conductance)
			    // Connected to shared via resistance and shared line connected right but
			    // not with other compartment
			    || (coordinate_system[y][x].switch_circuit_shared_conductance &&
			        coordinate_system[y][x].switch_shared_right && x != 255 &&
			        !coordinate_system[y][x + 1].switch_circuit_shared)
			    // Connected to shared line via resistance and shared line connected left but not
			    // with other compartment
			    || (coordinate_system[y][x].switch_circuit_shared_conductance && x != 0 &&
			        coordinate_system[y][x - 1].switch_shared_right &&
			        !coordinate_system[y][x - 1].switch_circuit_shared)
			    // Shared Line Closed but not connected
			    || (coordinate_system[y][x].switch_shared_right &&
			        !coordinate_system[y][x].switch_circuit_shared &&
			        !coordinate_system[y][x].switch_circuit_shared_conductance)) {
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
			// Connected to shared line directly and shared line connected right but not
			// with other compartment
			if (coordinate_system[y][x].switch_circuit_shared &&
			    coordinate_system[y][x].switch_shared_right && x != 255 &&
			    !coordinate_system[y][x + 1].switch_circuit_shared_conductance) {
				coordinate_system[y][x].switch_shared_right = false;
				coordinate_system[y][x].switch_circuit_shared = false;
				coordinate_system[y][x].switch_circuit_shared_conductance = false;
			}
			// Connected to shared line directly and shared line connected left but not
			// with other compartment
			if (coordinate_system[y][x].switch_circuit_shared && x != 0 &&
			    coordinate_system[y][x - 1].switch_shared_right &&
			    !coordinate_system[y][x - 1].switch_circuit_shared_conductance) {
				coordinate_system[y][x - 1].switch_shared_right = false;
				coordinate_system[y][x].switch_circuit_shared = false;
				coordinate_system[y][x].switch_circuit_shared_conductance = false;
			}
			// Connected to shared via resistance and shared line connected right but
			// not with other compartment
			if (coordinate_system[y][x].switch_circuit_shared_conductance &&
			    coordinate_system[y][x].switch_shared_right && x != 255 &&
			    !coordinate_system[y][x + 1].switch_circuit_shared) {
				coordinate_system[y][x].switch_shared_right = false;
				coordinate_system[y][x].switch_circuit_shared = false;
				coordinate_system[y][x].switch_circuit_shared_conductance = false;
			}
			// Connected to shared line via resistance and shared line connected left but not
			// with other compartment
			if (coordinate_system[y][x].switch_circuit_shared_conductance && x != 0 &&
			    coordinate_system[y][x - 1].switch_shared_right &&
			    !coordinate_system[y][x - 1].switch_circuit_shared) {
				coordinate_system[y][x - 1].switch_shared_right = false;
				coordinate_system[y][x].switch_circuit_shared = false;
				coordinate_system[y][x].switch_circuit_shared_conductance = false;
			}
			// Shared Line Closed but not connected
			if (coordinate_system[y][x].switch_shared_right &&
			    !coordinate_system[y][x].switch_circuit_shared &&
			    !coordinate_system[y][x].switch_circuit_shared_conductance) {
				coordinate_system[y][x].switch_shared_right = false;
			}
		}
	}
}


bool CoordinateSystem::short_circuit(size_t x_max) const
{
	for (size_t y = 0; y < 2; y++) {
		for (size_t x = 0; x < x_max; x++) {
			if (coordinate_system[y][x].switch_circuit_shared) {
				size_t x_temp = x;
				while (coordinate_system[y][x_temp].switch_shared_right) {
					if (coordinate_system[y][x_temp].switch_circuit_shared) {
						return true;
					}
					x_temp++;
				}
			}
		}
	}
	return false;
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

std::array<NeuronCircuit, 256>& CoordinateSystem::operator[](size_t y)
{
	return coordinate_system[y];
}


} // namespace grenade::vx::network::abstract