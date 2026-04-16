#pragma once
#include "grenade/common/topology_rewrite.h"
#include "hate/visibility.h"

namespace grenade::vx::network::abstract {

/**
 * Rewrite for uncalibrated signed synapse to two unsigned synapses.
 */
struct SYMBOL_VISIBLE UncalibratedSignedSynapseRewrite : public grenade::common::TopologyRewrite
{
	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param topology Linked topology
	 */
	UncalibratedSignedSynapseRewrite(std::shared_ptr<grenade::common::LinkedTopology> topology);

	virtual void operator()() const override;
};

} // namespace grenade::vx::network::abstract
