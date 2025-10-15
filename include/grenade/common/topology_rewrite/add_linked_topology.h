#pragma once
#include "grenade/common/topology_rewrite.h"
#include "hate/visibility.h"

namespace grenade::common {

/**
 * Rewrite, which adds a new layer of an empty linked topology with the previous linked topology as
 * reference.
 */
struct SYMBOL_VISIBLE AddLinkedTopologyRewrite : public TopologyRewrite
{
	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param topology Linked topology
	 */
	AddLinkedTopologyRewrite(std::shared_ptr<LinkedTopology> topology);

	virtual void operator()() const override;
};

} // namespace grenade::common
