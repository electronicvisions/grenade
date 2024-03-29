#include "grenade/vx/signal_flow/vertex/data_output.h"

#include <ostream>
#include <sstream>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

DataOutput::DataOutput(ConnectionType const input_type, size_t const size) :
    m_size(size), m_input_type()
{
	switch (input_type) {
		case ConnectionType::UInt32: {
			m_input_type = input_type;
			break;
		}
		case ConnectionType::UInt5: {
			m_input_type = input_type;
			break;
		}
		case ConnectionType::Int8: {
			m_input_type = input_type;
			break;
		}
		case ConnectionType::TimedSpikeToChipSequence: {
			m_input_type = input_type;
			if (m_size > 1) {
				throw std::runtime_error(
				    "DataOutput only supports size(1) for TimedSpikeToChipSequence.");
			}
			break;
		}
		case ConnectionType::TimedSpikeFromChipSequence: {
			m_input_type = input_type;
			if (m_size > 1) {
				throw std::runtime_error(
				    "DataOutput only supports size(1) for TimedSpikeFromChipSequence.");
			}
			break;
		}
		case ConnectionType::TimedMADCSampleFromChipSequence: {
			m_input_type = input_type;
			if (m_size > 1) {
				throw std::runtime_error(
				    "DataOutput only supports size(1) for TimedMADCSampleFromChipSequence.");
			}
			break;
		}
		default: {
			std::stringstream ss;
			ss << "Specified ConnectionType(" << input_type << ") to DataOutput not supported.";
			throw std::runtime_error(ss.str());
		}
	}
}

std::array<Port, 1> DataOutput::inputs() const
{
	return {Port(m_size, m_input_type)};
}

Port DataOutput::output() const
{
	auto const type = [&]() {
		switch (m_input_type) {
			case ConnectionType::UInt32: {
				return ConnectionType::DataUInt32;
			}
			case ConnectionType::UInt5: {
				return ConnectionType::DataUInt5;
			}
			case ConnectionType::Int8: {
				return ConnectionType::DataInt8;
			}
			case ConnectionType::TimedSpikeToChipSequence: {
				return ConnectionType::DataTimedSpikeToChipSequence;
			}
			case ConnectionType::TimedSpikeFromChipSequence: {
				return ConnectionType::DataTimedSpikeFromChipSequence;
			}
			case ConnectionType::TimedMADCSampleFromChipSequence: {
				return ConnectionType::DataTimedMADCSampleFromChipSequence;
			}
			default: {
				throw std::logic_error("Field m_input_type value of DataOutput not supported.");
			}
		}
	}();
	return Port(m_size, type);
}

std::ostream& operator<<(std::ostream& os, DataOutput const& config)
{
	os << "DataOutput(size: " << config.m_size << ", input_type: " << config.m_input_type << ")";
	return os;
}

bool DataOutput::operator==(DataOutput const& other) const
{
	return (m_size == other.m_size) && (m_input_type == other.m_input_type);
}

bool DataOutput::operator!=(DataOutput const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::signal_flow::vertex
