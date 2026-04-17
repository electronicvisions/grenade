#pragma once
#include "grenade/common/topology_rewrite.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/routing_result.h"
#include "hate/visibility.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Rewrite of mapped topology using routing result and adding
 * routing-related vertices (to a linked topology which was previously partially filled by the
 * PlacementRewrite).
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) RoutingRewrite : public grenade::common::TopologyRewrite
{
	RoutingResult routing_result;

	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param routing_results Routing result to use
	 * @param topology Linked topology
	 */
	RoutingRewrite(
	    RoutingResult routing_results, std::shared_ptr<grenade::common::LinkedTopology> topology);

	virtual void operator()() const override;
};

} // namespace abstract
} // namespace grenade::vx::network
