#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/common/geometry.h"

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/** Descriptor to be used to identify an inter-execution-instance projection on a network. */
struct GENPYBIND(inline_base("*")) InterExecutionInstanceProjectionOnNetwork
    : public halco::common::detail::RantWrapper<
          InterExecutionInstanceProjectionOnNetwork,
          size_t,
          (1ull << 32) - 1 /* arbitrary */,
          0>
{
	constexpr explicit InterExecutionInstanceProjectionOnNetwork(size_t const value = 0) :
	    base_t(value)
	{}
};

} // namespace grenade::vx::network

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::InterExecutionInstanceProjectionOnNetwork)

} // namespace std
