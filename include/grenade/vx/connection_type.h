#pragma once
#include <array>
#include <ostream>
#include "hate/type_traits.h"
#include "hate/visibility.h"

namespace grenade::vx {

/**
 * Type of data transfered by a connection.
 * Connections between vertices are only allowed if the input vertex output connection type is equal
 * to the output vertex input connection type.
 */
enum class ConnectionType
{
	SynapseInputLabel,   // PADI payload, 5b in ML, 6b in SNN
	Int8,                // CADC readout, PPU operation
	SynapticInput,       // Accumulated (analog) synaptic input for a neuron
	MembraneVoltage,     // Neuron membrane voltage for input of CADC readout
	DataOutputInt8,      // PPU computation or CADC readout value
	DataOutputUInt16,    // Spike label data
	CrossbarInputLabel,  // 14Bit label into crossbar
	CrossbarOutputLabel, // 14Bit label out of crossbar
	SynapseDriverInputLabel
};

std::ostream& operator<<(std::ostream& os, ConnectionType const& type) SYMBOL_VISIBLE;

/**
 * Only memory operations are allowed to connect between different execution instances.
 */
constexpr auto can_connect_different_execution_instances =
    std::array{ConnectionType::DataOutputInt8, ConnectionType::DataOutputUInt16};


namespace detail {

template <typename Vertex, typename InputVertex>
using has_supports_input_from =
    decltype(std::declval<Vertex>().supports_input_from(std::declval<InputVertex>()));

} // namespace detail

template <typename Vertex, typename InputVertex>
bool supports_input_from(Vertex const& vertex, InputVertex const& input)
{
	if constexpr (hate::is_detected_v<detail::has_supports_input_from, Vertex, InputVertex>) {
		return vertex.supports_input_from(input);
	}
	return true;
}

} // namespace grenade::vx
