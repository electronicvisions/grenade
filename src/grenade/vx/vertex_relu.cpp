#include "grenade/vx/vertex/relu.h"

#include "grenade/cerealization.h"

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

bool ReLU::operator==(ReLU const& other) const
{
	return m_size == other.m_size;
}

bool ReLU::operator!=(ReLU const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void ReLU::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::ReLU)
CEREAL_CLASS_VERSION(grenade::vx::vertex::ReLU, 0)
