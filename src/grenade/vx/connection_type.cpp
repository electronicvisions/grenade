#include "grenade/vx/connection_type.h"

namespace grenade::vx {

std::ostream& operator<<(std::ostream& os, ConnectionType const& type)
{
	switch (type) {
		case ConnectionType::SynapseInputLabel:
			return (os << "SynapseInputLabel");
		case ConnectionType::UInt5:
			return (os << "UInt5");
		case ConnectionType::Int8:
			return (os << "Int8");
		case ConnectionType::SynapticInput:
			return (os << "SynapticInput");
		case ConnectionType::MembraneVoltage:
			return (os << "MembraneVoltage");
		case ConnectionType::DataOutputUInt5:
			return (os << "DataOutputUInt5");
		case ConnectionType::DataOutputInt8:
			return (os << "DataOutputInt8");
		case ConnectionType::DataOutputUInt16:
			return (os << "DataOutputUInt16");
		case ConnectionType::DataInputUInt16:
			return (os << "DataInputUInt16");
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


} // namespace grenade::vx
