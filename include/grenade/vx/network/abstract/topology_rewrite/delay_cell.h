#pragma once
#include "grenade/common/topology_rewrite.h"
#include "hate/visibility.h"

namespace grenade::vx::network::abstract {

/**
 * Rewrite of populations with DelayCell cell type.
 * It replaces DelayCell populations with time domain to require no time domain and reassigns
 * separated time domains to the surrounding topology.
 */
struct SYMBOL_VISIBLE DelayCellRewrite : public grenade::common::TopologyRewrite
{
	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param topology Linked topology
	 */
	DelayCellRewrite(std::shared_ptr<grenade::common::LinkedTopology> topology);

	virtual void operator()() const override;
};

} // namespace grenade::vx::network::abstract
