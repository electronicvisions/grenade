#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "dapr/property_holder.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_on_compartment.h"

namespace grenade::vx::network::abstract {

void Environment::add(
    CompartmentOnNeuron const& compartment,
    MechanismOnCompartment const& mechanism,
    MechanismEnvironment const& environment)
{
	if (m_synaptic_connections.contains(compartment) &&
	    m_synaptic_connections.at(compartment).contains(mechanism)) {
		throw std::runtime_error("Environment already contains element to be added.");
	}
	m_synaptic_connections[compartment][mechanism] = environment;
}

void Environment::add_recordable(
    CompartmentOnNeuron const& compartment_a, CompartmentOnNeuron const& compartment_b)
{
	recordable_pairs.emplace(std::make_pair(compartment_a, compartment_b));
}

std::vector<CompartmentOnNeuron> Environment::get_recordable(
    CompartmentOnNeuron const& compartment) const
{
	std::vector<CompartmentOnNeuron> recordables;
	for (auto record_pair : recordable_pairs) {
		if (record_pair.first == compartment) {
			recordables.push_back(record_pair.second);
		} else if (record_pair.second == compartment) {
			recordables.push_back(record_pair.first);
		}
	}

	return recordables;
}

std::set<std::pair<CompartmentOnNeuron, CompartmentOnNeuron>> Environment::get_recordable_pairs()
    const
{
	return recordable_pairs;
}

void Environment::add(
    CompartmentOnNeuron const& compartment,
    std::map<MechanismOnCompartment, dapr::PropertyHolder<MechanismEnvironment>> const& environment)
{
	auto environment_copy = environment;
	m_synaptic_connections[compartment].merge(environment_copy);
	assert(environment_copy.empty());
}

std::map<MechanismOnCompartment, dapr::PropertyHolder<MechanismEnvironment>> Environment::get(
    CompartmentOnNeuron const& compartment) const
{
	if (!m_synaptic_connections.contains(compartment)) {
		return {};
	}
	return m_synaptic_connections.at(compartment);
}


} // namespace grenade::vx::network::abstract