#include "grenade/vx/network/abstract/multicompartment/placement/algorithm.h"
#include <log4cxx/logger.h>


namespace grenade::vx::network::abstract {

PlacementAlgorithm::PlacementAlgorithm() :
    logger(log4cxx::Logger::getLogger("grenade.MC.Placement.Algorithm"))
{
}


bool PlacementAlgorithm::valid(
    CoordinateSystem const& configuration, Neuron const& neuron, ResourceManager const& resources)
{
	Neuron neuron_constructed;

	// Check for dead end connections (only connected on one side or not connected to compartments)
	if (configuration.has_empty_connections(255)) {
		LOG4CXX_INFO(logger, "Validation failed: Empty connections.");
		return false;
	}
	// Check for double connections
	if (configuration.double_switch(255)) {
		LOG4CXX_INFO(logger, "Validation failed: Double switches set.");
		return false;
	}

	// Add compartments to neuron
	std::optional<CompartmentOnNeuron> compartment_temp;
	std::map<CompartmentOnNeuron, NumberTopBottom> resources_allocated;
	for (size_t y = 0; y < 2; y++) {
		for (size_t x = 0; x < 255; x++) {
			compartment_temp = configuration.coordinate_system[y][x].compartment;
			if (!compartment_temp) {
				continue;
			}
			if (resources_allocated.contains(compartment_temp.value())) {
				resources_allocated[compartment_temp.value()] += NumberTopBottom(1, 1 - y, y);
			} else {
				resources_allocated.emplace(compartment_temp.value(), NumberTopBottom(1, 1 - y, y));
			}
		}
	}

	// Check for resource requirements
	for (auto [compartment, neuron_circuits] : resources_allocated) {
		if (resources.get_config(compartment) > neuron_circuits) {
			LOG4CXX_INFO(logger, "Validation failed: Wrong number of resources.");
			return false;
		}
	}

	// Create Compartment-ID-Mapping
	std::map<CompartmentOnNeuron, CompartmentOnNeuron> compartment_mapping;
	for (auto [compartment, _] : resources_allocated) {
		compartment_mapping.emplace(
		    compartment, neuron_constructed.add_compartment(neuron.get(compartment)));
	}

	// Check for correct number of compartments
	if (neuron_constructed.num_compartments() != neuron.num_compartments()) {
		LOG4CXX_INFO(logger, "Validation failed: Wrong number of compartments.");
		return false;
	}
	LOG4CXX_TRACE(logger, "Neuron :\n" << neuron);
	LOG4CXX_TRACE(logger, "Neuron constructed:\n" << neuron_constructed);


	// Check inner connections
	for (size_t y = 0; y < 2; y++) {
		for (size_t x = 0; x < 255 - 1; x++) {
			if (configuration.get_compartment(x, y) != CompartmentOnNeuron() &&
			    (configuration.get_compartment(x, y) == configuration.get_compartment(x + 1, y)) &&
			    !configuration.connected_right(x, y)) {
				LOG4CXX_INFO(logger, "Validation failed: Missing inner connections.");
				return false;
			}
			if (configuration.get_compartment(x, y) != CompartmentOnNeuron() &&
			    (configuration.get_compartment(x, y) == configuration.get_compartment(x, 1 - y)) &&
			    !configuration.connected_top_bottom(x, y)) {
				LOG4CXX_INFO(logger, "Validation failed: Missing inner connections.");
				return false;
			}
			/*
			if (configuration.connected_right(x, y) &&
			        (configuration.get_compartment(x, y) == CompartmentOnNeuron()) ||
			    configuration.get_compartment(x + 1, y) == CompartmentOnNeuron()) {
			    LOG4CXX_INFO(logger, "Validation failed: Missing inner connections.");
			    return false;
			}
			if (configuration.connected_top_bottom(x, y) &&
			        (configuration.get_compartment(x, y) == CompartmentOnNeuron()) ||
			    configuration.get_compartment(x, 1 - y) == CompartmentOnNeuron()) {
			    LOG4CXX_INFO(logger, "Validation failed: Missing inner connections.");
			    return false;
			}
			*/
		}
	}

	// Add connections to neuron
	CompartmentOnNeuron compartment_temp_a, compartment_temp_b;
	std::set<std::pair<CompartmentOnNeuron, CompartmentOnNeuron>> connections_found;
	CompartmentConnectionConductance connection_dummy;
	std::vector<std::pair<size_t, size_t>> connected_coordinates;
	for (size_t y = 0; y < 2; y++) {
		for (size_t x = 0; x < 255; x++) {
			if (!configuration.coordinate_system[y][x].compartment) {
				continue; // TO-DO: Throw here?
			}
			connected_coordinates = configuration.connected_shared_conductance(x, y);
			compartment_temp_a =
			    compartment_mapping[configuration.coordinate_system[y][x].compartment.value()];
			for (auto [x_connect, y_connect] : connected_coordinates) {
				if (!configuration.coordinate_system[y_connect][x_connect].compartment) {
					continue; // TO-DO: Throw here?
				}
				compartment_temp_b =
				    compartment_mapping[configuration.coordinate_system[y_connect][x_connect]
				                            .compartment.value()];
				LOG4CXX_TRACE(
				    logger,
				    configuration.coordinate_system[y][x].compartment.value()
				        << "(y=" << y << "x=" << x << ")"
				        << " and "
				        << configuration.coordinate_system[y_connect][x_connect].compartment.value()
				        << "(y=" << y_connect << "x=" << x_connect << ")"
				        << "-> Connection found.");
				if (connections_found.contains(
				        std::make_pair(compartment_temp_a, compartment_temp_b)) ||
				    connections_found.contains(
				        std::make_pair(compartment_temp_b, compartment_temp_a))) {
					LOG4CXX_TRACE(logger, "Connection already on neuron.");
					continue;
				}
				neuron_constructed.add_compartment_connection(
				    compartment_temp_a, compartment_temp_b, connection_dummy);
				connections_found.emplace(std::make_pair(compartment_temp_a, compartment_temp_b));
			}
		}
	}

	LOG4CXX_TRACE(logger, "Neuron :\n" << neuron);
	LOG4CXX_TRACE(logger, "Neuron constructed:\n" << neuron_constructed);


	// Check for correct number of connections
	if (neuron_constructed.num_compartment_connections() != neuron.num_compartment_connections()) {
		LOG4CXX_INFO(logger, "Validation failed: Wrong number of connections.");
		LOG4CXX_DEBUG(
		    logger, "Found " << neuron_constructed.num_compartment_connections()
		                     << " connections instead of " << neuron.num_compartment_connections());
		return false;
	}

	// Check for isomorphism
	if (neuron.isomorphism(neuron_constructed).size() != neuron.num_compartments()) {
		LOG4CXX_INFO(logger, "Validation failed: Not isomorphic.");
		return false;
	}
	return true;
}

double PlacementAlgorithm::resource_efficient(
    CoordinateSystem const& configuration,
    Neuron const& neuron,
    ResourceManager const& resources) const
{
	// Calculate overall hardware requirements
	double required_counter = 0;
	for (auto it = neuron.compartments().first; it != neuron.compartments().second; it++) {
		required_counter += resources.get_config(*it).number_total;
	}

	// Find all neuron circuits with an assigned compartment
	double allocated_counter = 0;
	size_t lower_limit = 256;
	size_t upper_limit = 0;
	for (size_t x = 0; x < 255; x++) {
		if (configuration.coordinate_system[0][x].compartment != CompartmentOnNeuron()) {
			lower_limit = x;
		} else if (lower_limit != 256) {
			upper_limit = x;
		}
	}
	allocated_counter = 2 * (upper_limit - lower_limit);
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
	for (auto i = neuron.compartments().first; i != neuron.compartments().second; i++) {
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
		std::vector<std::pair<int, int>> coordinates = coordinate_system.find_neuron_circuits(i);

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