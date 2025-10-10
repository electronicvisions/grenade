#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/common/geometry.h"
#include "halco/common/geometry_numeric_limits.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(inline_base("*")) CompartmentConnectionOnNeuron
    : public halco::common::detail::BaseType<CompartmentConnectionOnNeuron, size_t>
{
	typedef value_type Value;

	constexpr CompartmentConnectionOnNeuron() = default;
	constexpr CompartmentConnectionOnNeuron(value_type const& value) : base_t(value) {}
};

} // namespace abstract
} // namespace grenade::vx::network

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::abstract::CompartmentConnectionOnNeuron)
HALCO_GEOMETRY_NUMERIC_LIMITS_CLASS(grenade::vx::network::abstract::CompartmentConnectionOnNeuron)

} // namespace std
