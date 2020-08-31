#pragma once
#include <array>
#include <ostream>
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
	UInt32,              // ArgMax output index
	UInt5,               // HAGEN input activation
	Int8,                // CADC readout, PPU operation
	SynapticInput,       // Accumulated (analog) synaptic input for a neuron
	MembraneVoltage,     // Neuron membrane voltage for input of CADC readout
	DataOutputInt8,      // PPU computation or CADC readout value
	DataOutputUInt5,     // HAGEN input activation data
	DataInputUInt16,     // Spike label data input
	DataOutputUInt16,    // Spike label data output
	DataOutputUInt32,    // Index data output
	CrossbarInputLabel,  // 14Bit label into crossbar
	CrossbarOutputLabel, // 14Bit label out of crossbar
	SynapseDriverInputLabel
};

std::ostream& operator<<(std::ostream& os, ConnectionType const& type) SYMBOL_VISIBLE;

/**
 * Only memory operations are allowed to connect between different execution instances.
 */
constexpr auto can_connect_different_execution_instances =
    std::array{ConnectionType::DataOutputUInt5, ConnectionType::DataOutputInt8,
               ConnectionType::DataInputUInt16, ConnectionType::DataOutputUInt16,
               ConnectionType::DataOutputUInt32};

} // namespace grenade::vx
