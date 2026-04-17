#pragma once
#include "grenade/common/vertex_port_type.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <array>
#include <iosfwd>

namespace grenade::vx {
namespace signal_flow GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

/**
 * Type of data transfered by a connection.
 * Connections between vertices are only allowed if the input vertex output connection type is equal
 * to the output vertex input connection type.
 */
enum class ConnectionType
{
	SynapseInputLabel,                   // PADI payload, 5b in ML, 6b in SNN
	UInt32,                              // ArgMax output index
	UInt5,                               // HAGEN input activation
	Int8,                                // CADC readout, PPU operation
	SynapticInput,                       // Accumulated (analog) synaptic input for a neuron
	MembraneVoltage,                     // Neuron membrane voltage for input of CADC readout
	TimedSpikeToChipSequence,            // Spike sequence to chip
	TimedSpikeFromChipSequence,          // Spike sequence from chip
	TimedMADCSampleFromChipSequence,     // MADC sample sequence from chip
	DataTimedSpikeToChipSequence,        // Spike sequence to chip data
	DataTimedSpikeFromChipSequence,      // Spike sequence from chip data
	DataTimedMADCSampleFromChipSequence, // MADC sample sequence from chip data
	DataInt8,                            // PPU computation or CADC readout value
	DataUInt5,                           // HAGEN input activation data
	DataUInt32,                          // Index data output
	CrossbarInputLabel,                  // 14Bit label into crossbar
	CrossbarOutputLabel,                 // 14Bit label out of crossbar
	SynapseDriverInputLabel,             // 11Bit label to synapse driver(s)
	ExternalAnalogSignal                 // Analog signal external to system, e.g. from/to pads
};

std::ostream& operator<<(std::ostream& os, ConnectionType const& type) SYMBOL_VISIBLE;

/**
 * Only memory operations are allowed to connect between different execution instances.
 */
constexpr auto can_connect_different_execution_instances = std::array{
    ConnectionType::DataUInt5,
    ConnectionType::DataInt8,
    ConnectionType::DataTimedSpikeToChipSequence,
    ConnectionType::DataTimedSpikeFromChipSequence,
    ConnectionType::DataUInt32,
    ConnectionType::DataTimedMADCSampleFromChipSequence};

/**
 * Port type to be used.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) VertexPortType : public grenade::common::VertexPortType
{
	ConnectionType type;

	VertexPortType(ConnectionType type);

	virtual std::unique_ptr<grenade::common::VertexPortType> copy() const override;
	virtual std::unique_ptr<grenade::common::VertexPortType> move() override;

protected:
	virtual bool is_equal_to(grenade::common::VertexPortType const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace signal_flow
} // namespace grenade::vx
