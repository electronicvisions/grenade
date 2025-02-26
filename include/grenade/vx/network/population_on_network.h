#pragma once
#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/population_on_execution_instance.h"

namespace grenade::vx { namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/** Descriptor to be used to identify a population on a network. */
struct GENPYBIND(inline_base("*ExecutionInstanceIDMixin*")) PopulationOnNetwork
    : public common::ExecutionInstanceIDMixin<PopulationOnNetwork, PopulationOnExecutionInstance>
{
	PopulationOnNetwork() = default;

	explicit PopulationOnNetwork(
	    PopulationOnExecutionInstance const& population,
	    common::ExecutionInstanceID const& execution_instance = common::ExecutionInstanceID()) :
	    mixin_t(population, execution_instance)
	{}

	explicit PopulationOnNetwork(enum_type const& e) : mixin_t(e) {}

	PopulationOnExecutionInstance toPopulationOnExecutionInstance() const
	{
		return This();
	}
};

} // namespace network
} // namespace grenade::vx

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::PopulationOnNetwork)

} // namespace std
