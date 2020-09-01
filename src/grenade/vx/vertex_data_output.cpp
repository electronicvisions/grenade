#include "grenade/vx/vertex/data_output.h"

#include <stdexcept>

namespace grenade::vx::vertex {

DataOutput::DataOutput(ConnectionType const input_type, size_t const size) :
    m_size(size), m_input_type()
{
	switch (input_type) {
		case ConnectionType::UInt5: {
			m_input_type = input_type;
			break;
		}
		case ConnectionType::Int8: {
			m_input_type = input_type;
			break;
		}
		case ConnectionType::DataOutputUInt16: {
			m_input_type = input_type;
			if (m_size > 1) {
				throw std::runtime_error("DataOutput only supports size(1) for DataOutputUInt16.");
			}
			break;
		}
		default: {
			throw std::runtime_error("Specified ConnectionType to DataOutput not supported.");
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
			case ConnectionType::UInt5: {
				return ConnectionType::DataOutputUInt5;
			}
			case ConnectionType::Int8: {
				return ConnectionType::DataOutputInt8;
			}
			case ConnectionType::DataOutputUInt16: {
				return ConnectionType::DataOutputUInt16;
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

} // namespace grenade::vx::vertex
