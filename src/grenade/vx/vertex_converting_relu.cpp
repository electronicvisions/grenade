#include "grenade/vx/vertex/converting_relu.h"

namespace grenade::vx::vertex {

ConvertingReLU::ConvertingReLU(size_t const size, uint32_t shift) : m_size(size), m_shift(shift) {}

uint32_t ConvertingReLU::get_shift() const
{
	return m_shift;
}

std::array<Port, 1> ConvertingReLU::inputs() const
{
	return {Port(m_size, ConnectionType::Int8)};
}

Port ConvertingReLU::output() const
{
	return Port(m_size, ConnectionType::UInt5);
}

std::ostream& operator<<(std::ostream& os, ConvertingReLU const& config)
{
	os << "ConvertingReLU(size: " << config.m_size << ", shift: " << config.m_shift << ")";
	return os;
}

} // namespace grenade::vx::vertex
