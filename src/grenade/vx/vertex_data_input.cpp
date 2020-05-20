#include "grenade/vx/vertex/data_input.h"

#include <stdexcept>

namespace grenade::vx::vertex {

DataInput::DataInput(ConnectionType const output_type, size_t const size) :
    m_size(size), m_output_type()
{
	switch (output_type) {
		case ConnectionType::Int8: {
			m_output_type = output_type;
			break;
		}
		case ConnectionType::CrossbarInputLabel: {
			if (size != 1) {
				throw std::runtime_error("CrossbarInputLabel only supports size 1.");
			}
			m_output_type = output_type;
			break;
		}
		default: {
			throw std::runtime_error("Specified ConnectionType to DataInput not supported.");
		}
	}
}

std::array<Port, 1> DataInput::inputs() const
{
	auto const type = [&]() {
		switch (m_output_type) {
			case ConnectionType::Int8: {
				return ConnectionType::DataOutputInt8;
			}
			case ConnectionType::CrossbarInputLabel: {
				return ConnectionType::DataInputUInt16;
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

} // namespace grenade::vx::vertex
