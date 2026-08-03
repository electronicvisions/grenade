#include "grenade/vx/network/abstract/multicompartment/resource_manager.h"

#include "dapr/unordered_map.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource.h"
#include <fstream>

namespace grenade::vx::network::abstract {

void ResourceManager::add_config(
    Neuron const& neuron,
    Neuron::ParameterSpace const& parameter_space,
    Environment const& environment)
{
	for (auto [compartment, compartment_parameter_space] : parameter_space.compartments) {
		add_config_compartment(compartment, neuron, compartment_parameter_space, environment);
	}

	m_recordable_pairs = environment.get_recordable_pairs();
}

void ResourceManager::remove_config(Neuron const& neuron)
{
	for (auto compartment : neuron.compartments()) {
		remove_config_compartment(compartment);
	}
}

NumberTopBottom const& ResourceManager::get_config(CompartmentOnNeuron const& compartment) const
{
	if (resource_map.find(compartment) == resource_map.end()) {
		throw std::invalid_argument("Invalid compartment: compartment not part of the resources");
	}
	return *(resource_map.at(compartment));
}

void ResourceManager::set_config(
    CompartmentOnNeuron const& compartment, NumberTopBottom const& config) const
{
	if (!resource_map.contains(compartment)) {
		throw std::invalid_argument("Invalid compartment: compartment not part of the resources");
	}
	*resource_map.at(compartment) = config;
}

std::vector<CompartmentOnNeuron> ResourceManager::get_compartments() const
{
	std::vector<CompartmentOnNeuron> compartments;
	for (auto [compartment, number] : resource_map) {
		compartments.push_back(compartment);
	}
	return compartments;
}

NumberTopBottom ResourceManager::get_total() const
{
	NumberTopBottom total;
	for (auto [_, resources] : resource_map) {
		total += *resources;
	}
	return total;
}

void ResourceManager::write_graphviz(
    std::string filename, Neuron const& neuron, std::string name, bool append)
{
	std::ofstream file;
	if (append) {
		file.open(filename, std::ofstream::app);
	} else {
		file.open(filename);
	}


	file << "graph " << name << " {\n";
	for (auto connection : neuron.compartment_connections()) {
		auto compartment_a = neuron.source(connection);
		auto compartment_b = neuron.target(connection);
		file << compartment_a << "(" << get_config(compartment_a) << ")"
		     << "--" << compartment_b << "(" << get_config(compartment_b) << ")"
		     << "\n";
	}
	file << "}\n";

	file.close();
}

std::set<std::pair<CompartmentOnNeuron, CompartmentOnNeuron>>
ResourceManager::get_recordable_pairs() const
{
	return m_recordable_pairs;
}

void ResourceManager::add_config_compartment(
    CompartmentOnNeuron const& compartment,
    Neuron const& neuron,
    Compartment::ParameterSpace const& parameter_space,
    Environment const& environment)
{
	// Map with hardware resources per mechanism on compartment
	std::map<MechanismOnCompartment, HardwareConstraints> hardware_constraints_on_mechanims =
	    neuron.get(compartment).get_hardware(parameter_space, environment.get(compartment));

	// Map with required resources per hardware constraint type
	dapr::UnorderedMap<HardwareResource, NumberTopBottom> accumulated_constraints;

	for (auto const& [_, constraints] : hardware_constraints_on_mechanims) {
		for (auto const& hardware_constraint : constraints) {
			auto const& resource = *hardware_constraint->resource;
			NumberTopBottom n_resources;
			if (accumulated_constraints.contains(resource)) {
				n_resources = accumulated_constraints.get(resource);
			}
			n_resources += hardware_constraint->numbers;
			accumulated_constraints.set(resource, n_resources);
		}
	}

	// Find number of required circuits by maximum of requested hardware resources
	size_t max_request_total = 0;
	size_t max_request_top = 0;
	size_t max_request_bottom = 0;
	for (auto const& [_, value] : accumulated_constraints) {
		if (value.number_total > max_request_total) {
			max_request_total = value.number_total;
		}
		if (value.number_bottom > max_request_bottom) {
			max_request_bottom = value.number_bottom;
		}
		if (value.number_top > max_request_top) {
			max_request_top = value.number_top;
		}
	}

	max_request_total = std::max(max_request_total, max_request_top + max_request_bottom);

	// Add configuration to resource map in ResourceManager
	resource_map.emplace(
	    compartment, NumberTopBottom(max_request_total, max_request_top, max_request_bottom));
}

void ResourceManager::remove_config_compartment(CompartmentOnNeuron const& compartment)
{
	if (!resource_map.contains(compartment)) {
		throw std::invalid_argument("Removed Compartment not in Resource Manager");
	}
	resource_map.erase(compartment);
}

std::ostream& operator<<(std::ostream& os, ResourceManager const& resources)
{
	os << "\n ResourceManager (\n";
	for (auto compartment : resources.get_compartments()) {
		os << "\t" << compartment << " : " << resources.get_config(compartment) << "\n";
	}
	os << ")";
	return os;
}

} // namespace grenade::vx::network::abstract
