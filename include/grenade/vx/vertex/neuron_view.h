#pragma once
#include <array>
#include <stddef.h>
#include <vector>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "hate/visibility.h"

namespace halco::hicann_dls::vx {
struct NeuronColumnOnDLS;
} // namespace halco::hicann_dls::vx

namespace grenade::vx::vertex {

/**
 * A view of neuron circuits.
 * TODO: add properties
 */
struct NeuronView
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef std::vector<halco::hicann_dls::vx::NeuronColumnOnDLS> neurons_type;

	/**
	 * Construct NeuronView with specified neurons.
	 * @param neurons Neuron locations
	 */
	explicit NeuronView(neurons_type const& neurons) SYMBOL_VISIBLE;

	neurons_type::const_iterator begin() const SYMBOL_VISIBLE;
	neurons_type::const_iterator end() const SYMBOL_VISIBLE;

	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

private:
	neurons_type m_neurons;
};

} // grenade::vx::vertex
