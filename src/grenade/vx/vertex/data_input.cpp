#include "grenade/vx/vertex/data_input.h"

#include "grenade/cerealization.h"
#include <ostream>
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
			case ConnectionType::UInt32: {
				return ConnectionType::DataOutputUInt32;
			}
			case ConnectionType::UInt5: {
				return ConnectionType::DataOutputUInt5;
			}
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
