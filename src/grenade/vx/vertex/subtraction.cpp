#include "grenade/vx/vertex/subtraction.h"

#include "grenade/cerealization.h"
#include <ostream>
#include <stdexcept>

namespace grenade::vx::vertex {

Subtraction::Subtraction(size_t const size) : m_size(size) {}

std::array<Port, 1> Subtraction::inputs() const
{
	return {Port(m_size, ConnectionType::Int8)};
}

Port Subtraction::output() const
{
	return Port(m_size, ConnectionType::Int8);
}

std::ostream& operator<<(std::ostream& os, Subtraction const& config)
{
	os << "Subtraction(size: " << config.m_size << ")";
	return os;
}

bool Subtraction::operator==(Subtraction const& other) const
{
	return m_size == other.m_size;
}

bool Subtraction::operator!=(Subtraction const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void Subtraction::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::Subtraction)
CEREAL_CLASS_VERSION(grenade::vx::vertex::Subtraction, 0)
