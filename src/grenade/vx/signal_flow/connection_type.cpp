#include "grenade/vx/signal_flow/connection_type.h"

#include <ostream>

namespace grenade::vx::signal_flow {

std::ostream& operator<<(std::ostream& os, ConnectionType const& type)
{
	switch (type) {
		case ConnectionType::SynapseInputLabel:
			return (os << "SynapseInputLabel");
		case ConnectionType::UInt5:
			return (os << "UInt5");
		case ConnectionType::UInt32:
			return (os << "UInt32");
		case ConnectionType::Int8:
			return (os << "Int8");
		case ConnectionType::SynapticInput:
			return (os << "SynapticInput");
		case ConnectionType::MembraneVoltage:
			return (os << "MembraneVoltage");
		case ConnectionType::TimedSpikeToChipSequence:
			return (os << "TimedSpikeToChipSequence");
		case ConnectionType::TimedSpikeFromChipSequence:
			return (os << "TimedSpikeFromChipSequence");
		case ConnectionType::TimedMADCSampleFromChipSequence:
			return (os << "TimedMADCSampleFromChipSequence");
		case ConnectionType::DataUInt5:
			return (os << "DataUInt5");
		case ConnectionType::DataUInt32:
			return (os << "DataUInt32");
		case ConnectionType::DataInt8:
			return (os << "DataInt8");
		case ConnectionType::DataTimedSpikeFromChipSequence:
			return (os << "DataTimedSpikeFromChipSequence");
		case ConnectionType::DataTimedSpikeToChipSequence:
			return (os << "DataTimedSpikeToChipSequence");
		case ConnectionType::DataTimedMADCSampleFromChipSequence:
			return (os << "DataTimedMADCSampleFromChipSequence");
		case ConnectionType::CrossbarInputLabel:
			return (os << "CrossbarInputLabel");
		case ConnectionType::CrossbarOutputLabel:
			return (os << "CrossbarOutputLabel");
		case ConnectionType::SynapseDriverInputLabel:
			return (os << "SynapseDriverInputLabel");
		default:
			throw std::logic_error("Ostream operator to given ConnectionType not implemented.");
	}
}


} // namespace grenade::vx::signal_flow
