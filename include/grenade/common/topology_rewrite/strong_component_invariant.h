#pragma once
#include "grenade/common/topology_rewrite.h"
#include "hate/visibility.h"

namespace grenade::common {

/**
 * Rewrite, which initializes the linked topology with strong component invariant vertices of the
 * reference topology.
 * For each strong component invariant, a vertex is created.
 * Edges between the vertices are added for all strong component invariant transitions via edges
 * present in the reference topology, while each edge between strong component invariants is only
 * added once.
 */
struct SYMBOL_VISIBLE StrongComponentInvariantRewrite : public grenade::common::TopologyRewrite
{
	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param topology Linked topology
	 */
	StrongComponentInvariantRewrite(std::shared_ptr<grenade::common::LinkedTopology> topology);

	virtual void operator()() const override;
};

} // namespace grenade::common
