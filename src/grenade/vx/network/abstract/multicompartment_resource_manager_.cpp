#include "grenade/vx/network/abstract/multicompartment_resource_manager.h"


namespace grenade::vx::network::abstract {

CompartmentOnNeuron ResourceManager::add_config(
    CompartmentOnNeuron const& compartment, Neuron const& neuron, Environment const& environment)
{
	// Map with HardwareResources per Mechanism on Compartment
	std::map<MechanismOnCompartment, HardwareResourcesWithConstraints>
	    hardware_resources_with_constraints_on_mechanims =
	        neuron.get(compartment).get_hardware(compartment, environment);
	// Circuit Configuration
	NumberTopBottom neuron_circuit_config;
	// Two Vectors to count Requestes Resources to find maximum later (Vectors instead of map since
	// PropertyHolder is neither comparable nor hashable)
	std::vector<common::PropertyHolder<HardwareResource>> resource_request_counter_hardware = {
	    HardwareResourceCapacity(), HardwareResourceSynapticInputExitatory(),
	    HardwareResourceSynapticInputInhibitory()};
	std::vector<NumberTopBottom> resource_request_counter_numbers = {
	    NumberTopBottom(0, 0, 0), NumberTopBottom(0, 0, 0), NumberTopBottom(0, 0, 0)};

	// Iterate over all Mechanisms over all Constraints to set Number of Min_Top and Min_Bottom
	for (auto [Key, Value] : hardware_resources_with_constraints_on_mechanims) {
		// Iterate over all HardwareResources of a Mechanism
		for (auto hardware_resource : Value.resources) {
			// Check which hardware Resource is requested and increase counter
			for (size_t i = 0; i < resource_request_counter_hardware.size(); i++) {
				if (typeid(*(resource_request_counter_hardware.at(i))) ==
				    typeid(*hardware_resource)) {
					resource_request_counter_numbers.at(i).number_total++;
				}
			}
		}
		// Iterate over all HardwareConstraints of a Mechanism
		for (auto hardware_constraint : Value.constraints) {
			// Check which HardwareConstraint is requested and increase counter
			for (size_t i = 0; i < resource_request_counter_hardware.size(); i++) {
				if (typeid(*(resource_request_counter_hardware.at(i))) ==
				    typeid(*((*hardware_constraint).resource))) {
					resource_request_counter_numbers.at(i).number_top +=
					    (*hardware_constraint).numbers.number_top;
					resource_request_counter_numbers.at(i).number_bottom +=
					    (*hardware_constraint).numbers.number_bottom;
				}
			}
		}
	}

	// Find Number of Reqired Circuits by maximum of requested Hardware-Resources
	size_t max_request_total = 0;
	size_t max_request_top = 0;
	size_t max_request_bottom = 0;
	for (auto const& Value : resource_request_counter_numbers) {
		if (Value.number_total > max_request_total) {
			max_request_total = Value.number_total;
		}
		if (Value.number_bottom > max_request_bottom) {
			max_request_bottom = Value.number_bottom;
		}
		if (Value.number_top > max_request_top) {
			max_request_top = Value.number_top;
		}
	}
	// Add Configuration to Resource Map in ResourceManager
	neuron_circuit_config = NumberTopBottom(max_request_total, max_request_top, max_request_bottom);
	resource_map.emplace(compartment, neuron_circuit_config);
	m_total += NumberTopBottom(max_request_total, max_request_top, max_request_bottom);
	return compartment;
}

void ResourceManager::remove_config(CompartmentOnNeuron const& compartment)
{
	if (!resource_map.contains(compartment)) {
		throw std::invalid_argument("Removed Compartment not in Resource Manager");
	}
	resource_map.erase(compartment);
}

NumberTopBottom const& ResourceManager::get_config(CompartmentOnNeuron const& compartment) const
{
	if (resource_map.find(compartment) == resource_map.end()) {
		throw std::invalid_argument("Invalid Compartment: No Configuration");
	}
	return *(resource_map.at(compartment));
}

void ResourceManager::add_config(Neuron const& neuron, Environment const& environment)
{
	for (auto i = neuron.compartment_iterators().first; i != neuron.compartment_iterators().second;
	     i++) {
		add_config(*i, neuron, environment);
	}
}

NumberTopBottom const& ResourceManager::get_total() const
{
	return m_total;
}

} // namespace grenade::vx::network::abstract