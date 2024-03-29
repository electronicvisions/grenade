#include "grenade/vx/signal_flow/vertex/external_input.h"

#include <ostream>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

ExternalInput::ExternalInput(ConnectionType const output_type, size_t const size) :
    m_size(size), m_output_type(output_type)
{
	switch (output_type) {
		case ConnectionType::DataUInt32: {
			m_output_type = output_type;
			break;
		}
		case ConnectionType::DataUInt5: {
			m_output_type = output_type;
			break;
		}
		case ConnectionType::DataInt8: {
			m_output_type = output_type;
			break;
		}
		case ConnectionType::DataTimedSpikeToChipSequence: {
			if (m_size > 1) {
				throw std::runtime_error(
				    "ExternalInput only supports size(1) for DataTimedSpikeToChipSequence.");
			}
			m_output_type = output_type;
			break;
		}
		case ConnectionType::DataTimedSpikeFromChipSequence: {
			m_output_type = output_type;
			if (m_size > 1) {
				throw std::runtime_error(
				    "ExternalInput only supports size(1) for DataTimedSpikeFromChipSequence.");
			}
			break;
		}
		case ConnectionType::DataTimedMADCSampleFromChipSequence: {
			m_output_type = output_type;
			if (m_size > 1) {
				throw std::runtime_error(
				    "ExternalInput only supports size(1) for DataTimedMADCSampleFromChipSequence.");
			}
			break;
		}
		default: {
			std::stringstream ss;
			ss << "Specified ConnectionType(" << output_type << ") to ExternalInput not supported.";
			throw std::runtime_error(ss.str());
		}
	}
}

Port ExternalInput::output() const
{
	return Port(m_size, m_output_type);
}

std::ostream& operator<<(std::ostream& os, ExternalInput const& config)
{
	os << "ExternalInput(size: " << config.m_size << ", output_type: " << config.m_output_type
	   << ")";
	return os;
}

bool ExternalInput::operator==(ExternalInput const& other) const
{
	return (m_size == other.m_size) && (m_output_type == other.m_output_type);
}

bool ExternalInput::operator!=(ExternalInput const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::signal_flow::vertex
