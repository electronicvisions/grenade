#include "grenade/vx/network/abstract/multicompartment_placement_algorithm_evolutionary.h"

namespace grenade::vx::network::abstract {

void PlacementAlgorithmEvolutionary::construct_neuron(size_t result_index, size_t x_max)
{
	// Current Placement Result
	AlgorithmResult& result_temp = m_results_temp.at(result_index);
	result_temp = m_results.at(result_index);

	// Empty Neuron for construction
	Neuron& neuron_constructed = m_constructed_neurons.at(result_index);
	neuron_constructed = Neuron();

	// Empty Resources for construction
	std::map<CompartmentOnNeuron, NumberTopBottom>& resources_constructed =
	    m_constructed_resources.at(result_index);
	resources_constructed.clear();

	// Iterate over Coordinate-System to find Compartments with set switches and add to constructed
	// Neuron and constructed resources
	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			if (result_temp.coordinate_system.coordinate_system[y][x].compartment ==
			        CompartmentOnNeuron() &&
			    result_temp.coordinate_system.connected(x, y)) {
				// Add Compartment to Neuron
				Compartment temp_compartment;
				CompartmentOnNeuron temp_descriptor =
				    neuron_constructed.add_compartment(temp_compartment);
				// Assign ID to Coordinate System
				NumberTopBottom number_neuron_circuits =
				    result_temp.coordinate_system.assign_compartment_adjacent(
				        x, y, temp_descriptor);
				// Add allocated resources to resource map
				resources_constructed.emplace(temp_descriptor, number_neuron_circuits);
				m_constructed_resources_total.at(result_index) += number_neuron_circuits;
			}
		}
	}

	// Add Compartment Connections
	CompartmentConnectionConductance connection_temp;
	for (size_t x = 0; x < x_max && x < 128 - 1; x++) {
		for (size_t y = 0; y < 2; y++) {
			if (result_temp.coordinate_system.connected_right_shared(x, y) &&
			    !neuron_constructed.neighbour(
			        result_temp.coordinate_system.coordinate_system[y][x].compartment,
			        result_temp.coordinate_system.coordinate_system[y][x + 1].compartment)) {
				neuron_constructed.add_compartment_connection(
				    result_temp.coordinate_system.coordinate_system[y][x].compartment,
				    result_temp.coordinate_system.coordinate_system[y][x + 1].compartment,
				    connection_temp);
			}
		}
	}
}
double PlacementAlgorithmEvolutionary::fitness_number_compartments(
    size_t result_index, Neuron const& neuron) const
{
	double compartments_constructed = m_constructed_neurons.at(result_index).num_compartments();
	double compartments = neuron.num_compartments();
	if (compartments_constructed <= compartments) {
		return (1 - ((compartments - compartments_constructed) / compartments));
	} else {
		return (1 - ((compartments_constructed - compartments) / compartments));
	}
}
double PlacementAlgorithmEvolutionary::fitness_number_compartment_connections(
    size_t result_index, Neuron const& neuron) const
{
	double connections_constructed =
	    m_constructed_neurons.at(result_index).num_compartment_connections();
	double connections = neuron.num_compartment_connections();
	if (connections_constructed <= connections) {
		return (1 - ((connections - connections_constructed) / connections));
	} else {
		return (1 - ((connections_constructed - connections) / connections));
	}
}
double PlacementAlgorithmEvolutionary::fitness_resources_total(
    size_t result_index, ResourceManager const& resources) const
{
	NumberTopBottom constructed_resources_total = m_constructed_resources_total.at(result_index);
	NumberTopBottom required_resources_total = resources.get_total();
	double fitness = 0;

	// Calculate weighted fitnesses by difference of total resource reqirements
	if (constructed_resources_total.number_total <= required_resources_total.number_total) {
		fitness += 0.5 * (1 - ((required_resources_total.number_total -
		                        constructed_resources_total.number_total) /
		                       required_resources_total.number_total));
	} else {
		fitness += 0.5 * (1 - ((constructed_resources_total.number_total -
		                        required_resources_total.number_total) /
		                       required_resources_total.number_total));
	}
	// Calculate weighted fitnesses by difference of top row resource reqirements
	if (constructed_resources_total.number_top <= required_resources_total.number_top) {
		fitness +=
		    0.25 *
		    (1 - ((required_resources_total.number_top - constructed_resources_total.number_top) /
		          required_resources_total.number_top));
	} else {
		fitness +=
		    0.25 *
		    (1 - ((constructed_resources_total.number_top - required_resources_total.number_top) /
		          required_resources_total.number_top));
	}
	// Calculate weighted fitnesses by difference of bottom row resource reqirements
	if (constructed_resources_total.number_bottom <= required_resources_total.number_bottom) {
		fitness += 0.25 * (1 - ((required_resources_total.number_bottom -
		                         constructed_resources_total.number_bottom) /
		                        required_resources_total.number_bottom));
	} else {
		fitness += 0.25 * (1 - ((constructed_resources_total.number_bottom -
		                         required_resources_total.number_bottom) /
		                        required_resources_total.number_bottom));
	}
	return fitness;
}
double PlacementAlgorithmEvolutionary::fitness_isomorphism(
    size_t result_index, Neuron const& neuron, ResourceManager const& resources) const
{
	double number_compartments_largest_subgraph =
	    isomorphism_resources_subgraph(
	        neuron, m_constructed_neurons.at(result_index), resources,
	        m_constructed_resources.at(result_index))
	        .first;
	double number_compartments = neuron.num_compartments();
	if (number_compartments_largest_subgraph < number_compartments) {
		return (
		    1 -
		    ((number_compartments - number_compartments_largest_subgraph) / number_compartments));
	} else {
		return (
		    1 -
		    ((number_compartments_largest_subgraph - number_compartments) / number_compartments));
	}
}

AlgorithmResult PlacementAlgorithmEvolutionary::run(
    CoordinateSystem const&, Neuron const&, ResourceManager const&)
{
	AlgorithmResult dummy_result;
	return dummy_result;
}


bool PlacementAlgorithmEvolutionary::valid(
    size_t result_index, size_t x_max, Neuron const& neuron, ResourceManager const& resources)
{
	if (x_max > 128) {
		throw std::invalid_argument("x_max > 128, invalid");
	}

	// Final result to assign compartments
	AlgorithmResult& result = m_results.at(result_index);
	// Temporary result to perform tests on
	AlgorithmResult& result_temp = m_results_temp.at(result_index);
	result_temp = result;

	// Check for empty Connections (Dead Ends)
	if (result_temp.coordinate_system.has_empty_connections(128) ||
	    result_temp.coordinate_system.has_double_connections(128)) {
		return false;
	}

	// Construct Neuron from State of Switches in Coordinate System
	Neuron neuron_constructed;
	std::map<CompartmentOnNeuron, NumberTopBottom> resources_constructed;
	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			if (result_temp.coordinate_system.coordinate_system[y][x].compartment ==
			        CompartmentOnNeuron() &&
			    result_temp.coordinate_system.connected(x, y)) {
				// Add Compartment to Neuron
				Compartment temp_compartment;
				CompartmentOnNeuron temp_descriptor =
				    neuron_constructed.add_compartment(temp_compartment);
				// Assign ID to Coordinate System
				NumberTopBottom number_neuron_circuits =
				    result_temp.coordinate_system.assign_compartment_adjacent(
				        x, y, temp_descriptor);
				// Add allocated resources to resource map
				resources_constructed.emplace(temp_descriptor, number_neuron_circuits);
			}
		}
	}

	// Check for number of compartments
	if (neuron_constructed.num_compartments() != neuron.num_compartments()) {
		return false;
	}

	// Add Compartment Connections
	CompartmentConnectionConductance connection_temp;
	for (size_t x = 0; x < x_max && x < 128 - 1; x++) {
		for (size_t y = 0; y < 2; y++) {
			if (result_temp.coordinate_system.connected_left_shared(x, y)) {
				neuron_constructed.add_compartment_connection(
				    result_temp.coordinate_system.coordinate_system[y][x].compartment,
				    result_temp.coordinate_system.coordinate_system[y][x].compartment,
				    connection_temp);
			}
		}
	}

	// Check for number of compartment-connections
	if (neuron_constructed.num_compartment_connections() != neuron.num_compartment_connections()) {
		return false;
	}

	// Check for isomorphism
	std::pair<bool, std::map<CompartmentOnNeuron, CompartmentOnNeuron>> isomorphism =
	    isomorphism_resources(neuron, neuron_constructed, resources, resources_constructed);
	if (!isomorphism.first) {
		return false;
	}

	// Assing correct compartment-IDs to coordinate system
	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			result_temp.coordinate_system.coordinate_system[y][x].compartment =
			    isomorphism
			        .second[result_temp.coordinate_system.coordinate_system[y][x].compartment];
		}
	}

	// Assign result with temporary result
	result_temp.finished = true;
	result = result_temp;
	return true;
}


double PlacementAlgorithmEvolutionary::fitness(
    size_t result_index, size_t x_max, Neuron const& neuron, ResourceManager const& resources)
{
	construct_neuron(result_index, x_max);
	double fitness_num_compartments = fitness_number_compartments(result_index, neuron);
	double fitness_num_compartments_connections =
	    fitness_number_compartment_connections(result_index, neuron);
	double fitness_resources = fitness_resources_total(result_index, resources);

	double fitness = 0.5 * fitness_num_compartments + 0.2 * fitness_num_compartments_connections +
	                 0.3 * fitness_resources;

	return fitness;
}
} // namespace grenade::vx::network::abstract