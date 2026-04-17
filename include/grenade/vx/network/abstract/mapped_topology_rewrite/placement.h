#pragma once
#include "grenade/common/topology_rewrite.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/placement_result.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/padi.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Rewrite using placement result to create mapped topology by adding all placed
 * hardware entities (to a previously empty linked topology).
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) PlacementRewrite : public grenade::common::TopologyRewrite
{
	PlacementResult placement_result;

	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param placement_result Placement result to use
	 * @param topology Linked topology
	 */
	PlacementRewrite(
	    PlacementResult placement_result,
	    std::shared_ptr<grenade::common::LinkedTopology> topology);

	virtual void operator()() const override;
};

} // namespace abstract
} // namespace grenade::vx::network
