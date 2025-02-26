#pragma once
#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/plasticity_rule_on_execution_instance.h"

namespace grenade::vx { namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/** Descriptor to be used to identify a plasticity rule on a network. */
struct GENPYBIND(inline_base("*ExecutionInstanceIDMixin*")) PlasticityRuleOnNetwork
    : public common::
          ExecutionInstanceIDMixin<PlasticityRuleOnNetwork, PlasticityRuleOnExecutionInstance>
{
	PlasticityRuleOnNetwork() = default;

	explicit PlasticityRuleOnNetwork(
	    PlasticityRuleOnExecutionInstance const& population,
	    common::ExecutionInstanceID const& execution_instance = common::ExecutionInstanceID()) :
	    mixin_t(population, execution_instance)
	{}

	explicit PlasticityRuleOnNetwork(enum_type const& e) : mixin_t(e) {}

	PlasticityRuleOnExecutionInstance toPlasticityRuleOnExecutionInstance() const
	{
		return This();
	}
};

} // namespace network
} // namespace grenade::vx

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::PlasticityRuleOnNetwork)

} // namespace std
