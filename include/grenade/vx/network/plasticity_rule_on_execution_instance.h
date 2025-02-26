#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/common/geometry.h"

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/** Descriptor to be used to identify a plasticity rule on an execution instance. */
struct GENPYBIND(inline_base("*")) PlasticityRuleOnExecutionInstance
    : public halco::common::detail::RantWrapper<
          PlasticityRuleOnExecutionInstance,
          size_t,
          (1ull << 32) - 1 /* arbitrary */,
          0>
{
	constexpr explicit PlasticityRuleOnExecutionInstance(size_t const value = 0) : base_t(value) {}
};

} // namespace network
} // namespace grenade::vx

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::PlasticityRuleOnExecutionInstance)

} // namespace std
