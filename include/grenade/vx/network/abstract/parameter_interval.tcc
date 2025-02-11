#pragma once
#include "grenade/vx/network/abstract/parameter_interval.h"
#include <ostream>

namespace grenade::vx::network::abstract {
// Constructor
template <class T>
ParameterInterval<T>::ParameterInterval(T lower_limit, T upper_limit)
{
	if (lower_limit > upper_limit) {
		throw std::invalid_argument("Non valid Interval Limits");
	}
	m_lower_limit = lower_limit;
	m_upper_limit = upper_limit;
}

// Check if parameter is in Interval
template <class T>
bool ParameterInterval<T>::contains(T const& parameter) const
{
	return (parameter <= m_upper_limit && parameter >= m_lower_limit);
}

// Get for limit Values
template <class T>
T const& ParameterInterval<T>::get_lower() const
{
	return (m_lower_limit);
}
template <class T>
T const& ParameterInterval<T>::get_upper() const
{
	return (m_upper_limit);
}

// Print Interval
template <class T>
std::ostream& operator<<(std::ostream& os, ParameterInterval<T> const& interval)
{
	return os << "[" << interval.get_lower() << ", " << interval.get_upper() << "]";
}
} // namespace grenade::vx::network::abstract