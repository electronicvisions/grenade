#include "grenade/vx/signal_flow/vertex/relu.h"

#include <ostream>

namespace grenade::vx::signal_flow::vertex {

ReLU::ReLU(size_t const size) : m_size(size) {}

std::array<Port, 1> ReLU::inputs() const
{
	return {Port(m_size, ConnectionType::Int8)};
}

Port ReLU::output() const
{
	return Port(m_size, ConnectionType::Int8);
}

std::ostream& operator<<(std::ostream& os, ReLU const& config)
{
	os << "ReLU(size: " << config.m_size << ")";
	return os;
}

bool ReLU::operator==(ReLU const& other) const
{
	return m_size == other.m_size;
}

bool ReLU::operator!=(ReLU const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::signal_flow::vertex
