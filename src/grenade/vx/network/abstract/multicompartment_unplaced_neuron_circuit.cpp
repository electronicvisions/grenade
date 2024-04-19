#include "grenade/vx/network/abstract/multicompartment_unplaced_neuron_circuit.h"

namespace grenade::vx::network::abstract {

void UnplacedNeuronCircuit::set_status()
{
	m_switch_status = 0;
	m_switch_status |= (switch_shared_right << 0);
	m_switch_status |= (switch_right << 1);
	m_switch_status |= (switch_top_bottom << 2);
	m_switch_status |= (switch_circuit_shared << 3);
	m_switch_status |= (switch_circuit_shared_conductance << 4);
}

int UnplacedNeuronCircuit::get_status()
{
	set_status();
	return m_switch_status;
}

void UnplacedNeuronCircuit::set_switches()
{
	switch_shared_right = (m_switch_status >> 0) & 1;
	switch_right = (m_switch_status >> 1) & 1;
	switch_top_bottom = (m_switch_status >> 2) & 1;
	switch_circuit_shared = (m_switch_status >> 3) & 1;
	switch_circuit_shared_conductance = (m_switch_status >> 4) & 1;
}


bool UnplacedNeuronCircuit::operator++()
{
	bool first = true;
	while ((switch_circuit_shared && switch_circuit_shared_conductance) || first) {
		set_status();
		// std::cout << "Switch Status0: " << m_switch_status << std::endl;
		if (m_switch_status == 23) {
			m_switch_status = 0;
			return false;
		} else {
			first = false;
			m_switch_status++;

			set_switches();
		}
	}
	// std::cout << "Switch Status1: " << m_switch_status << std::endl;
	return true;
}

bool UnplacedNeuronCircuit::operator+=(size_t number)
{
	set_status();
	m_switch_status = (m_switch_status + number) % 24;
	set_switches();
	return true;
}

} // namespace grenade::vx::network::abstract