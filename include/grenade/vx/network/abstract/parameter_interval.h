#pragma once
#include <iosfwd>

namespace grenade::vx::network {
template <class T>
struct ParameterInterval
{
	// Constructor
	ParameterInterval() = default;
	ParameterInterval(T lower_limit, T upper_limit);

	// Check if parameter is in Interval
	bool contains(T const& parameter) const;

	// Get for limit Values
	T const& get_lower() const;
	T const& get_upper() const;

	// Equality-Operator and Inequality-Operator
	bool operator==(ParameterInterval const& other) const = default;
	bool operator!=(ParameterInterval const& other) const = default;

private:
	T m_lower_limit{};
	T m_upper_limit{};
};

// Print Interval
template <typename T>
std::ostream& operator<<(std::ostream& os, ParameterInterval<T> const& interval);

} // namespace grenade::vx::network

#include "grenade/vx/network/abstract/parameter_interval.tcc"