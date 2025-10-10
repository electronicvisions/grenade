#include "grenade/vx/network/abstract/multicompartment_placement_algorithm.h"


namespace grenade::vx::network::abstract {

// Validation of a configuration with given target neuron and required resources
bool PlacementAlgorithm::valid(
    CoordinateSystem const& configuration, Neuron const& neuron, ResourceManager const& resources)
{
	/*std::cout << "\n______________________________Validation______________________________\n\n";*/
	Neuron neuron_constructed;
	std::map<CompartmentOnNeuron, NumberTopBottom> configuration_compartments;

	// Iterate over CoordinateSystem
	for (int y = 0; y < 2; y++) {
		for (int x = 0; x < 255; x++) {
			if (!configuration.coordinate_system[y][x].compartment) {
				continue;
			}
			if (configuration_compartments.find(
			        configuration.coordinate_system[y][x].compartment.value()) !=
			    configuration_compartments.end()) {
				configuration_compartments[configuration.coordinate_system[y][x]
				                               .compartment.value()] +=
				    NumberTopBottom(1, 1 - y, y);
			} else if (configuration.coordinate_system[y][x].compartment) {
				configuration_compartments.emplace(
				    configuration.coordinate_system[y][x].compartment.value(),
				    NumberTopBottom(1, 1 - y, y));
			}
		}
	}


	// Check if Resource Requirements are met
	for (auto [key, value] : configuration_compartments) {
		if (resources.get_config(key).number_total > value.number_total ||
		    resources.get_config(key).number_top > value.number_top ||
		    resources.get_config(key).number_bottom > value.number_bottom) {
			std::cout << "Not Valid: Resource Requriments not met" << std::endl;
			return false;
		}
	}
	// std::cout << "Valid: resource Requirements" << std::endl;

	// Check inner connections
	for (auto [key, value] : configuration_compartments) {
		std::vector<std::pair<int, int>> coordinates = configuration.find_compartment(key);
		for (auto i : coordinates) {
			if (configuration.coordinate_system[i.second][i.first + 1].compartment == key &&
			    configuration.coordinate_system[i.second][i.first].switch_right == false) {
				std::cout << "Not Valid: Inner Connections missing" << std::endl;
				return false;
			}
			if (configuration.coordinate_system[1 - i.second][i.first].compartment == key &&
			    (configuration.coordinate_system[1 - i.second][i.first].switch_top_bottom ==
			         false ||
			     configuration.coordinate_system[i.second][i.first].switch_top_bottom == false)) {
				std::cout << "Not Valid: Inner Connections missing" << std::endl;
				return false;
			}
		}
	}
	// std::cout << "Valid: inner Connections" << std::endl;


	// Build Neuron and compare with Input Neuron
	// Add Compartments
	std::map<CompartmentOnNeuron, CompartmentOnNeuron> id_map;
	for (auto [key, value] : configuration_compartments) {
		id_map.emplace(key, neuron_constructed.add_compartment(neuron.get(key)));
	}
	// std::cout << "TEST: Compartemnts number" << id_map.size() << std::endl;
	// Add CompartmentConnections
	for (auto [key, value] : configuration_compartments) {
		std::vector<std::pair<int, int>> coordinates = configuration.find_compartment(key);
		for (auto i : coordinates) {
			// Check for connection right
			if ((configuration.coordinate_system[i.second][i.first].switch_circuit_shared &&
			     configuration.coordinate_system[i.second][i.first].switch_shared_right &&
			     configuration.coordinate_system[i.second][i.first + 1]
			         .switch_circuit_shared_conductance) ||
			    (configuration.coordinate_system[i.second][i.first]
			         .switch_circuit_shared_conductance &&
			     configuration.coordinate_system[i.second][i.first].switch_shared_right &&
			     configuration.coordinate_system[i.second][i.first + 1].switch_circuit_shared)) {
				bool connected = false;
				for (auto j = neuron_constructed.adjacent_compartments(id_map[key]).first;
				     j != neuron_constructed.adjacent_compartments(id_map[key]).second; j++) {
					if (*j == id_map[configuration.coordinate_system[i.second][i.first + 1]
					                     .compartment.value()]) {
						connected = true;
						break;
					}
				}
				if (!connected) {
					CompartmentConnectionConductance connection;
					neuron_constructed.add_compartment_connection(
					    id_map[key],
					    id_map[configuration.coordinate_system[i.second][i.first + 1]
					               .compartment.value()],
					    connection);
				}
			}
			// Check for connection left
			else if (
			    (configuration.coordinate_system[i.second][i.first].switch_circuit_shared &&
			     configuration.coordinate_system[i.second][i.first - 1].switch_shared_right &&
			     configuration.coordinate_system[i.second][i.first - 1]
			         .switch_circuit_shared_conductance) ||
			    (configuration.coordinate_system[i.second][i.first]
			         .switch_circuit_shared_conductance &&
			     configuration.coordinate_system[i.second][i.first - 1].switch_shared_right &&
			     configuration.coordinate_system[i.second][i.first - 1].switch_circuit_shared)) {
				// Add CompartmentConnection if not already connected
				bool connected = false;
				for (auto j = neuron_constructed.adjacent_compartments(id_map[key]).first;
				     j != neuron_constructed.adjacent_compartments(id_map[key]).second; j++) {
					if (*j == id_map[configuration.coordinate_system[i.second][i.first - 1]
					                     .compartment.value()]) {
						connected = true;
						break;
					}
				}
				if (!connected) {
					CompartmentConnectionConductance connection;
					neuron_constructed.add_compartment_connection(
					    id_map[key],
					    id_map[configuration.coordinate_system[i.second][i.first - 1]
					               .compartment.value()],
					    connection);
				}
			}
		}
	}
	if (neuron.num_compartment_connections() != neuron_constructed.num_compartment_connections() ||
	    neuron.num_compartments() != neuron_constructed.num_compartments()) {
		std::cout << "Not Valid: Number Compartments/CompartmentConnections wrong" << std::endl;
		return false;
	}
	// std::cout << "Valid: Numbers Compartments/CompartmentConnections" << std::endl;
	return neuron.isomorphism(neuron_constructed).size() == neuron.num_compartments();
}

double PlacementAlgorithm::resource_efficient(
    CoordinateSystem const& configuration,
    Neuron const& neuron,
    ResourceManager const& resources) const
{
	// Calculate overall hardware requirements
	double required_counter = 0;
	for (auto it = neuron.compartment_iterators().first;
	     it != neuron.compartment_iterators().second; it++) {
		required_counter += resources.get_config(*it).number_total;
	}

	// Find all neuron circuits with an assigned compartment
	double allocated_counter = 0;
	for (int y = 0; y < 2; y++) {
		for (int x = 0; x < 256; x++) {
			if (configuration.coordinate_system[y][x].compartment != CompartmentOnNeuron()) {
				allocated_counter++;
			}
		}
	}
	return allocated_counter / required_counter;
}

// Isomorphism which checks for resource requirments to find right permutation
std::pair<size_t, std::map<CompartmentOnNeuron, CompartmentOnNeuron>>
PlacementAlgorithm::isomorphism_resources(
    Neuron const& neuron,
    Neuron const& neuron_build,
    ResourceManager const& resources,
    std::map<CompartmentOnNeuron, NumberTopBottom> const& resources_build) const
{
	std::map<CompartmentOnNeuron, CompartmentOnNeuron> id_mapping;
	size_t null_compartments_min = neuron.num_compartments();

	auto const callback =
	    [&null_compartments_min, &id_mapping](
	        size_t null_compartments,
	        std::map<CompartmentOnNeuron, CompartmentOnNeuron> const& compartment_mapping,
	        std::map<CompartmentOnNeuron, CompartmentOnNeuron> const&) -> bool {
		// If no exact match found continue to search
		if (null_compartments > 0) {
			return true;
		}

		// If exact match found map compartment-IDs and stop isomorphism search
		id_mapping = compartment_mapping;
		null_compartments_min = null_compartments;
		return false;
	};

	auto const compartment_equivalent = [&neuron, &neuron_build, &resources, &resources_build](
	                                        CompartmentOnNeuron compartment_build,
	                                        CompartmentOnNeuron compartment) -> bool {
		return resources.get_config(compartment) <= resources_build.at(compartment_build);
	};

	neuron.isomorphism(neuron_build, callback, compartment_equivalent);

	return std::make_pair(null_compartments_min, id_mapping);
}

// Isomorphism which checks for resource requirments to find right permutation
std::pair<size_t, std::map<CompartmentOnNeuron, CompartmentOnNeuron>>
PlacementAlgorithm::isomorphism_resources_subgraph(
    Neuron const& neuron,
    Neuron const& neuron_build,
    ResourceManager const& resources,
    std::map<CompartmentOnNeuron, NumberTopBottom> const& resources_build) const
{
	// std::cout << "Checking Isomorphism" << std::endl;
	std::map<CompartmentOnNeuron, CompartmentOnNeuron> id_mapping;
	size_t null_compartments_min = neuron.num_compartments();

	auto const callback =
	    [&id_mapping, &null_compartments_min](
	        size_t null_compartments,
	        std::map<CompartmentOnNeuron, CompartmentOnNeuron> const& compartment_mapping,
	        std::map<CompartmentOnNeuron, CompartmentOnNeuron> const&) -> bool {
		if (null_compartments > 0) {
			return true;
		}

		id_mapping = compartment_mapping;
		null_compartments_min = null_compartments;
		return false;
	};

	auto const compartment_equivalent = [&neuron, &neuron_build, &resources, &resources_build](
	                                        CompartmentOnNeuron compartment_build,
	                                        CompartmentOnNeuron compartment) -> bool {
		return resources.get_config(compartment) <= resources_build.at(compartment_build);
	};

	neuron.isomorphism_subgraph(neuron_build, callback, compartment_equivalent);

	return std::make_pair(null_compartments_min, id_mapping);
}

// Converts CoordinateSystem Into LogicalSystem
halco::hicann_dls::vx::LogicalNeuronCompartments
PlacementAlgorithm::convert_to_logical_compartments(
    CoordinateSystem const& coordinate_system, Neuron const& neuron)
{
	// All CompartmentIDs
	std::vector<CompartmentOnNeuron> compartment_ids;
	for (auto i = neuron.compartment_iterators().first; i != neuron.compartment_iterators().second;
	     i++) {
		compartment_ids.push_back(*i);
	}

	// Result
	std::map<
	    halco::hicann_dls::vx::CompartmentOnLogicalNeuron,
	    std::vector<halco::hicann_dls::vx::AtomicNeuronOnLogicalNeuron>>
	    Compartments;

	// Iterate over all Compartment-IDs
	int keyCounter = 0;
	for (auto i : compartment_ids) {
		std::vector<std::pair<int, int>> coordinates = coordinate_system.find_compartment(i);

		std::vector<halco::hicann_dls::vx::AtomicNeuronOnLogicalNeuron> atomic_neurons;
		// Iterate over all Locations of Compartment
		for (auto j : coordinates) {
			auto neuron_column = halco::hicann_dls::vx::NeuronColumnOnLogicalNeuron(j.first);
			auto neuron_row = halco::hicann_dls::vx::NeuronRowOnLogicalNeuron(j.second);
			atomic_neurons.push_back(
			    halco::hicann_dls::vx::AtomicNeuronOnLogicalNeuron(neuron_row, neuron_column));
		}
		Compartments.emplace(
		    halco::hicann_dls::vx::CompartmentOnLogicalNeuron(keyCounter++), atomic_neurons);
	}

	return halco::hicann_dls::vx::LogicalNeuronCompartments(Compartments);
}

} // namespace grenade::vx::network::abstract