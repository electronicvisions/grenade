#pragma once
#include "grenade/common/topology_rewrite.h"
#include "hate/visibility.h"

namespace grenade::vx::network::abstract {

/**
 * Trivial plasticity rule rewrite, which applies connection on executor from observables (and uses
 * default connection if no observables are connected).
 */
struct SYMBOL_VISIBLE PlasticityRuleRewrite : public grenade::common::TopologyRewrite
{
	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param topology Linked topology
	 */
	PlasticityRuleRewrite(std::shared_ptr<grenade::common::LinkedTopology> topology);

	virtual void operator()() const override;
};

} // namespace grenade::vx::network::abstract
