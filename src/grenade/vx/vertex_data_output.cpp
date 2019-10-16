#include "grenade/vx/vertex/data_output.h"

#include <stdexcept>

namespace grenade::vx::vertex {

DataOutput::DataOutput(ConnectionType const input_type, size_t const size) :
    m_size(size), m_input_type()
{
	switch (input_type) {
		case ConnectionType::SynapseInputLabel: {
			m_input_type = input_type;
			break;
		}
		case ConnectionType::Int8: {
			m_input_type = input_type;
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
			case ConnectionType::SynapseInputLabel: {
				return ConnectionType::DataOutputUInt5;
			}
			case ConnectionType::Int8: {
				return ConnectionType::DataOutputInt8;
			}
			default: {
				throw std::logic_error("Field m_input_type value of DataOutput not supported.");
			}
		}
	}();
	return Port(m_size, type);
}

} // namespace grenade::vx::vertex
