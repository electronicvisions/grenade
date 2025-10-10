#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/common/geometry.h"
#include "halco/common/geometry_numeric_limits.h"


namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(inline_base("*")) CompartmentOnNeuron
    : public halco::common::detail::BaseType<CompartmentOnNeuron, size_t>
{
	typedef value_type Value;

	constexpr CompartmentOnNeuron() = default;
	constexpr CompartmentOnNeuron(value_type const& value) : base_t(value) {}
};

} // namespace abstract
} // namespace grenade::vx::network

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::abstract::CompartmentOnNeuron)
HALCO_GEOMETRY_NUMERIC_LIMITS_CLASS(grenade::vx::network::abstract::CompartmentOnNeuron)

} // namespace std
