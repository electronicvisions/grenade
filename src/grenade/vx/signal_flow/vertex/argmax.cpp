#include "grenade/vx/signal_flow/vertex/argmax.h"

#include <limits>
#include <ostream>
#include <sstream>

namespace grenade::vx::signal_flow::vertex {

ArgMax::ArgMax(size_t const size, ConnectionType const type) : m_size(size), m_type()
{
	switch (type) {
		case ConnectionType::UInt32: {
			m_type = type;
			break;
		}
		case ConnectionType::UInt5: {
			m_type = type;
			break;
		}
		case ConnectionType::Int8: {
			m_type = type;
			break;
		}
		default: {
			std::stringstream ss;
			ss << "Specified ConnectionType(" << type << ") to ArgMax not supported.";
			throw std::runtime_error(ss.str());
		}
	}
	if (m_size > std::numeric_limits<uint32_t>::max()) {
		throw std::runtime_error(
		    "Specified size(" + std::to_string(m_size) + ") to ArgMax not representable.");
	}
}

std::array<Port, 1> ArgMax::inputs() const
{
	return {Port(m_size, m_type)};
}

Port ArgMax::output() const
{
	return Port(1, ConnectionType::UInt32);
}

std::ostream& operator<<(std::ostream& os, ArgMax const& config)
{
	os << "ArgMax(size: " << config.m_size << ", type: " << config.m_type << ")";
	return os;
}

bool ArgMax::operator==(ArgMax const& other) const
{
	return (m_size == other.m_size) && (m_type == other.m_type);
}

bool ArgMax::operator!=(ArgMax const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::signal_flow::vertex
