#include "grenade/vx/signal_flow/port_restriction.h"

#include "grenade/vx/signal_flow/port.h"
#include <ostream>
#include <stdexcept>

namespace grenade::vx::signal_flow {

PortRestriction::PortRestriction(size_t const min_in, size_t const max_in)
{
	if (min_in > max_in) {
		throw std::runtime_error(
		    "PortRestriction supplied min(" + std::to_string(min_in) + ") > supplied max(" +
		    std::to_string(max_in) + ") parameter.");
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

} // namespace grenade::vx::signal_flow
