#include "grenade/common/topology_rewrite.h"
#include "grenade/common/linked_topology.h"

namespace grenade::common {

TopologyRewrite::TopologyRewrite(std::shared_ptr<LinkedTopology> topology) :
    m_topology(std::move(topology))
{
}

LinkedTopology& TopologyRewrite::get_topology() const
{
	if (!m_topology) {
		throw std::logic_error("Unexpected access to moved-from object.");
	}
	return *m_topology;
}

} // namespace grenade::common
