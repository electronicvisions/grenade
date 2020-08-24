#include "grenade/vx/port_restriction.h"

#include "grenade/cerealization.h"
#include "grenade/vx/port.h"
#include <stdexcept>

namespace grenade::vx {

PortRestriction::PortRestriction(size_t const min_in, size_t const max_in)
{
	if (min_in > max_in) {
		throw std::runtime_error("PortRestriction supplied min > supplied max parameter.");
	}
	m_min = min_in;
	m_max = max_in;
}

bool PortRestriction::operator==(PortRestriction const& other) const
{
	return m_min == other.m_min && m_max == other.m_max;
}

bool PortRestriction::operator!=(PortRestriction const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, PortRestriction const& data)
{
	os << "PortRestriction(min: " << data.m_min << ", max: " << data.m_max << ")";
	return os;
}

size_t PortRestriction::size() const
{
	return m_max - m_min + 1;
}

bool PortRestriction::is_restriction_of(Port const& port) const
{
	return m_max <= port.size;
}

size_t PortRestriction::min() const
{
	return m_min;
}

size_t PortRestriction::max() const
{
	return m_max;
}

template <typename Archive>
void PortRestriction::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_min);
	ar(m_max);
}

} // namespace grenade::vx

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::PortRestriction)
CEREAL_CLASS_VERSION(grenade::vx::PortRestriction, 0)
