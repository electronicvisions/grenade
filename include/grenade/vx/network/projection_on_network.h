#pragma once
#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/projection_on_execution_instance.h"

namespace grenade::vx { namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/** Descriptor to be used to identify a projection on a network. */
struct GENPYBIND(inline_base("*ExecutionInstanceIDMixin*")) ProjectionOnNetwork
    : public common::ExecutionInstanceIDMixin<ProjectionOnNetwork, ProjectionOnExecutionInstance>
{
	ProjectionOnNetwork() = default;

	explicit ProjectionOnNetwork(
	    ProjectionOnExecutionInstance const& projection,
	    common::ExecutionInstanceID const& execution_instance = common::ExecutionInstanceID()) :
	    mixin_t(projection, execution_instance)
	{}

	explicit ProjectionOnNetwork(enum_type const& e) : mixin_t(e) {}

	ProjectionOnExecutionInstance toProjectionOnExecutionInstance() const
	{
		return This();
	}
};

} // namespace network
} // namespace grenade::vx

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::ProjectionOnNetwork)

} // namespace std
