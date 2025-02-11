#pragma once
#include "grenade/common/topology_rewrite.h"
#include "hate/visibility.h"

namespace grenade::common {

/**
 * Rewrite, which replaces the linked topology by an identical copy of the reference topology
 * with identity inter topology hyper edges in-between.
 */
struct SYMBOL_VISIBLE IdentityReplacementTopologyRewrite : public TopologyRewrite
{
	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param topology Linked topology
	 */
	IdentityReplacementTopologyRewrite(std::shared_ptr<LinkedTopology> topology);

	virtual void operator()() const override;
};

} // namespace grenade::common
