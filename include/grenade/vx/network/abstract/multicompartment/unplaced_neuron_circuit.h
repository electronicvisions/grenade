#pragma once
#include "grenade/vx/genpybind.h"
#include <iostream>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Stores configuration of the switches of a neuron circuit that interconnect neuron circuits.
 */
struct GENPYBIND(visible) UnplacedNeuronCircuit
{
	bool switch_shared_right = 0;
	bool switch_right = 0;
	bool switch_top_bottom = 0;
	bool switch_circuit_shared = 0;
	bool switch_circuit_shared_conductance = 0;

	/**
	 * Get the status integer value representing the state of all five switches.
	 */
	int GENPYBIND(hidden) get_status();

	/**
	 * Increments the switch status and set switches accordingly.
	 */
	bool GENPYBIND(hidden) operator++();

	/**
	 * Set switches according to the value of the switch status.
	 */
	void GENPYBIND(hidden) set_switches();

	/**
	 * Increments the switch status by the given value and set switches accordingly.
	 */
	bool GENPYBIND(hidden) operator+=(size_t number);

	// Comparison to other switch configuration.
	bool GENPYBIND(hidden) operator==(UnplacedNeuronCircuit const& other) const = default;
	bool GENPYBIND(hidden) operator!=(UnplacedNeuronCircuit const& other) const = default;

private:
	/**
	 * Set a status integer value that represents the current state of all five switches.
	 */
	void GENPYBIND(hidden) set_status();

	int m_switch_status = 0;
};

} // namespace abstract
} // namespace grenade::vx::network
