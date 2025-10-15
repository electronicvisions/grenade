#include "grenade/common/topology_rewrite/add_linked_topology.h"

namespace grenade::common {

AddLinkedTopologyRewrite::AddLinkedTopologyRewrite(std::shared_ptr<LinkedTopology> topology) :
    TopologyRewrite(std::move(topology))
{
}

void AddLinkedTopologyRewrite::operator()() const
{
	auto const old_topology = std::make_shared<LinkedTopology>(std::move(get_topology()));
	get_topology() = LinkedTopology(old_topology);
}

} // namespace grenade::common
