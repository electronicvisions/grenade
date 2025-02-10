#pragma once
#include "grenade/common/genpybind.h"
#include "grenade/common/linked_topology.h"
#include "hate/visibility.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Rewrite altering linked topology.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) TopologyRewrite
{
	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param topology Linked topology
	 */
	TopologyRewrite(std::shared_ptr<LinkedTopology> topology);

	/**
	 * Rewrite topology.
	 */
	virtual void operator()() const = 0;

protected:
	LinkedTopology& get_topology() const;

private:
	std::shared_ptr<LinkedTopology> m_topology;
};

} // namespace common
} // namespace grenade
