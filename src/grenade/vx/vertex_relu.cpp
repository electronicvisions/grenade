#include "grenade/vx/vertex/relu.h"

namespace grenade::vx::vertex {

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

} // namespace grenade::vx::vertex
