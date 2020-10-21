#include "grenade/vx/vertex/data_input.h"

#include "grenade/cerealization.h"
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace grenade::vx::vertex {

DataInput::DataInput(ConnectionType const output_type, size_t const size) :
    m_size(size), m_output_type()
{
	switch (output_type) {
		case ConnectionType::UInt32: {
			m_output_type = output_type;
			break;
		}
		case ConnectionType::UInt5: {
			m_output_type = output_type;
			break;
		}
		case ConnectionType::Int8: {
			m_output_type = output_type;
			break;
		}
		case ConnectionType::TimedSpikeSequence: {
			if (m_size > 1) {
				throw std::runtime_error("DataInput only supports size(1) for TimedSpikeSequence.");
			}
			m_output_type = output_type;
			break;
		}
		case ConnectionType::TimedSpikeFromChipSequence: {
			m_output_type = output_type;
			if (m_size > 1) {
				throw std::runtime_error(
				    "DataInput only supports size(1) for TimedSpikeFromChipSequence.");
			}
			break;
		}
		case ConnectionType::TimedMADCSampleFromChipSequence: {
			m_output_type = output_type;
			if (m_size > 1) {
				throw std::runtime_error(
				    "DataInput only supports size(1) for TimedMADCSampleFromChipSequence.");
			}
			break;
		}
		default: {
			std::stringstream ss;
			ss << "Specified ConnectionType(" << output_type << ") to DataInput not supported.";
			throw std::runtime_error(ss.str());
		}
	}
}

std::array<Port, 1> DataInput::inputs() const
{
	auto const type = [&]() {
		switch (m_output_type) {
			case ConnectionType::UInt32: {
				return ConnectionType::DataUInt32;
			}
			case ConnectionType::UInt5: {
				return ConnectionType::DataUInt5;
			}
			case ConnectionType::Int8: {
				return ConnectionType::DataInt8;
			}
			case ConnectionType::TimedSpikeSequence: {
				return ConnectionType::DataTimedSpikeSequence;
			}
			case ConnectionType::TimedSpikeFromChipSequence: {
				return ConnectionType::DataTimedSpikeFromChipSequence;
			}
			case ConnectionType::TimedMADCSampleFromChipSequence: {
				return ConnectionType::DataTimedMADCSampleFromChipSequence;
			}
			default: {
				throw std::logic_error("Field m_output_type value of DataInput not supported.");
			}
		}
	}();
	return {Port(m_size, type)};
}

Port DataInput::output() const
{
	return Port(m_size, m_output_type);
}

std::ostream& operator<<(std::ostream& os, DataInput const& config)
{
	os << "DataInput(size: " << config.m_size << ", output_type: " << config.m_output_type << ")";
	return os;
}

bool DataInput::operator==(DataInput const& other) const
{
	return (m_size == other.m_size) && (m_output_type == other.m_output_type);
}

bool DataInput::operator!=(DataInput const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void DataInput::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
	ar(m_output_type);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::DataInput)
CEREAL_CLASS_VERSION(grenade::vx::vertex::DataInput, 0)
