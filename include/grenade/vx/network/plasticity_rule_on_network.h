#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/common/geometry.h"

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/** Descriptor to be used to identify a plasticity rule on a network. */
struct GENPYBIND(inline_base("*")) PlasticityRuleOnNetwork
    : public halco::common::detail::BaseType<PlasticityRuleOnNetwork, size_t>
{
	constexpr explicit PlasticityRuleOnNetwork(value_type const value = 0) : base_t(value) {}
};

} // namespace grenade::vx::network

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::PlasticityRuleOnNetwork)

} // namespace std
