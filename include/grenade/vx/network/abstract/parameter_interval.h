#pragma once
#include "ccalix/types.h"
#include "grenade/vx/genpybind.h"
#include "haldls/vx/v3/cadc.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"
#include <iosfwd>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {


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

extern template struct SYMBOL_VISIBLE ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue>;
extern template struct SYMBOL_VISIBLE ParameterInterval<haldls::vx::v3::CADCSampleQuad::Value>;
extern template struct SYMBOL_VISIBLE ParameterInterval<ccalix::CapacitanceInFarad>;
extern template struct SYMBOL_VISIBLE ParameterInterval<ccalix::TimeInS>;

typedef ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue> AnalogValueInterval
    GENPYBIND(opaque(false));
typedef ParameterInterval<haldls::vx::v3::CADCSampleQuad::Value> CADCInterval
    GENPYBIND(opaque(false));
typedef ParameterInterval<ccalix::CapacitanceInFarad> CapacitanceInterval GENPYBIND(opaque(false));
typedef ParameterInterval<double> ParameterIntervalDouble GENPYBIND(opaque(false));
typedef ParameterInterval<ccalix::TimeInS> TimeInterval GENPYBIND(opaque(false));


} // namespace abstract
} // namespace grenade::vx::network

#include "grenade/vx/network/abstract/parameter_interval.tcc"
