#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "dapr/property_holder.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_on_compartment.h"

namespace grenade::vx::network::abstract {

void Environment::add(
    grenade::common::CompartmentOnNeuron const& compartment,
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
    grenade::common::CompartmentOnNeuron const& compartment_a,
    grenade::common::CompartmentOnNeuron const& compartment_b)
{
	recordable_pairs.emplace(std::make_pair(compartment_a, compartment_b));
}

std::vector<grenade::common::CompartmentOnNeuron> Environment::get_recordable(
    grenade::common::CompartmentOnNeuron const& compartment) const
{
	std::vector<grenade::common::CompartmentOnNeuron> recordables;
	for (auto record_pair : recordable_pairs) {
		if (record_pair.first == compartment) {
			recordables.push_back(record_pair.second);
		} else if (record_pair.second == compartment) {
			recordables.push_back(record_pair.first);
		}
	}

	return recordables;
}

std::set<std::pair<grenade::common::CompartmentOnNeuron, grenade::common::CompartmentOnNeuron>>
Environment::get_recordable_pairs() const
{
	return recordable_pairs;
}

void Environment::add(
    grenade::common::CompartmentOnNeuron const& compartment,
    std::map<MechanismOnCompartment, dapr::PropertyHolder<MechanismEnvironment>> const& environment)
{
	auto environment_copy = environment;
	m_synaptic_connections[compartment].merge(environment_copy);
	assert(environment_copy.empty());
}

std::map<MechanismOnCompartment, dapr::PropertyHolder<MechanismEnvironment>> Environment::get(
    grenade::common::CompartmentOnNeuron const& compartment) const
{
	if (!m_synaptic_connections.contains(compartment)) {
		return {};
	}
	return m_synaptic_connections.at(compartment);
}


} // namespace grenade::vx::network::abstract