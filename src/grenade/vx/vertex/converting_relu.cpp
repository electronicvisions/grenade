#include "grenade/vx/vertex/converting_relu.h"

#include "grenade/cerealization.h"

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

bool ConvertingReLU::operator==(ConvertingReLU const& other) const
{
	return (m_size == other.m_size) && (m_shift == other.m_shift);
}

bool ConvertingReLU::operator!=(ConvertingReLU const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void ConvertingReLU::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
	ar(m_shift);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::ConvertingReLU)
CEREAL_CLASS_VERSION(grenade::vx::vertex::ConvertingReLU, 0)
