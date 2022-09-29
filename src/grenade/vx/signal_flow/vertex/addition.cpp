#include "grenade/vx/signal_flow/vertex/addition.h"

#include "grenade/cerealization.h"
#include <ostream>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

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

bool Addition::operator==(Addition const& other) const
{
	return m_size == other.m_size;
}

bool Addition::operator!=(Addition const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void Addition::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::Addition)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::Addition, 0)
