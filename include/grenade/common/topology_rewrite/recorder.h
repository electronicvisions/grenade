#pragma once
#include "grenade/common/topology_rewrite.h"
#include "hate/visibility.h"

namespace grenade::common {

/**
 * Recorder rewrite, which splits recorders along their source vertices.
 */
struct SYMBOL_VISIBLE RecorderTopologyRewrite : public TopologyRewrite
{
	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param topology Linked topology
	 */
	RecorderTopologyRewrite(std::shared_ptr<LinkedTopology> topology);

	virtual void operator()() const override;
};

} // namespace grenade::common
