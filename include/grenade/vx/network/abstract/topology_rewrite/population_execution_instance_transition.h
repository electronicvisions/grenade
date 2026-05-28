#pragma once
#include "grenade/common/topology_rewrite.h"
#include "hate/visibility.h"

namespace grenade::vx::network::abstract {

/**
 * Rewrite of execution instance transitions of populations.
 * It replaces the transition by a SpikeRecorder on the source execution instance and a population
 * with ExternalSourceNeuron on the target execution instance.
 */
struct SYMBOL_VISIBLE PopulationExecutionInstanceTransitionRewrite
    : public grenade::common::TopologyRewrite
{
	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param topology Linked topology
	 */
	PopulationExecutionInstanceTransitionRewrite(
	    std::shared_ptr<grenade::common::LinkedTopology> topology);

	virtual void operator()() const override;
};

} // namespace grenade::vx::network::abstract
