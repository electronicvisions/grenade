#include "grenade/vx/vertex/addition.h"

#include <stdexcept>

namespace grenade::vx::vertex {

Addition::Addition(size_t const size) : m_size(size) {}

std::array<Port, 1> Addition::inputs() const
{
	return {Port(m_size, ConnectionType::Int8)};
}

Port Addition::output() const
{
	return Port(m_size, ConnectionType::Int8);
}

std::ostream& operator<<(std::ostream& os, Addition const& config)
{
	os << "Addition(size: " << config.m_size << ")";
	return os;
}

} // namespace grenade::vx::vertex
