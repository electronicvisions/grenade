#pragma once
#include "grenade/common/topology_rewrite.h"
#include "hate/visibility.h"

namespace grenade::common {

/**
 * Projection rewrite, which splits projection along post-synaptic populations.
 */
struct SYMBOL_VISIBLE ProjectionTopologyRewrite : public TopologyRewrite
{
	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param topology Linked topology
	 */
	ProjectionTopologyRewrite(std::shared_ptr<LinkedTopology> topology);

	virtual void operator()() const override;
};

} // namespace grenade::common
