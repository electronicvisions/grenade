#include "grenade/vx/vertex/argmax.h"

#include "grenade/cerealization.h"
#include <limits>

namespace grenade::vx::vertex {

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
			throw std::runtime_error("Specified ConnectionType to ArgMax not supported.");
		}
	}
	if (m_size > std::numeric_limits<uint32_t>::max()) {
		throw std::runtime_error("Specified size to ArgMax not representable.");
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

template <typename Archive>
void ArgMax::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
	ar(m_type);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::ArgMax)
CEREAL_CLASS_VERSION(grenade::vx::vertex::ArgMax, 0)
