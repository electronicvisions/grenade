#pragma once
#include <array>
#include <stddef.h>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "hate/visibility.h"


namespace grenade::vx::vertex {

/**
 * Readout of membrane voltages via the CADC.
 */
struct CADCMembraneReadoutView
{
	constexpr static bool can_connect_different_execution_instances = false;

	/**
	 * Construct CADCMembraneReadoutView with specified size.
	 * @param size Number of read-out neuron membranes
	 * @throws std::out_of_range On size larger than the amount of neurons on an execution instance
	 */
	explicit CADCMembraneReadoutView(size_t const size) SYMBOL_VISIBLE;

	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

private:
	size_t m_size;
};

} // grenade::vx::vertex
