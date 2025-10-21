#include "grenade/vx/network/abstract/multicompartment_environment.h"

namespace grenade::vx::network::abstract {

void Environment::add(
    CompartmentOnNeuron const& compartment, SynapticInputEnvironment const& synaptic_input)
{
	if (m_synaptic_connections.find(compartment) == m_synaptic_connections.end()) {
		std::vector<dapr::PropertyHolder<SynapticInputEnvironment>> synaptic_inputs;
		synaptic_inputs.push_back(synaptic_input);
		m_synaptic_connections.emplace(std::make_pair(compartment, synaptic_inputs));
	} else {
		m_synaptic_connections.at(compartment).push_back(synaptic_input);
	}
}

void Environment::add(
    CompartmentOnNeuron const& compartment,
    std::vector<dapr::PropertyHolder<SynapticInputEnvironment>> const& synaptic_inputs)
{
	if (!m_synaptic_connections.contains(compartment)) {
		m_synaptic_connections.emplace(std::make_pair(compartment, synaptic_inputs));
	} else {
		for (auto synaptic_input : synaptic_inputs) {
			m_synaptic_connections.at(compartment).push_back(synaptic_input);
		}
	}
}
std::vector<dapr::PropertyHolder<SynapticInputEnvironment>> Environment::get(
    CompartmentOnNeuron const& compartment) const
{
	if (!m_synaptic_connections.contains(compartment)) {
		return std::vector<dapr::PropertyHolder<SynapticInputEnvironment>>();
	}
	return m_synaptic_connections.at(compartment);
}


} // namespace grenade::vx::network::abstract