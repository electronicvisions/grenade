#include "grenade/vx/network/placed_logical/requires_routing.h"

#include "grenade/vx/network/placed_atomic/requires_routing.h"
#include "grenade/vx/network/placed_logical/build_atomic_network.h"
#include "grenade/vx/network/placed_logical/build_connection_routing.h"
#include "grenade/vx/network/placed_logical/network.h"
#include "grenade/vx/network/placed_logical/network_graph.h"

namespace grenade::vx::network::placed_logical {

bool requires_routing(std::shared_ptr<Network> const& current, NetworkGraph const& old)
{
	assert(current);
	assert(old.get_network());

	// check if projection count changed
	if (current->projections.size() != old.get_network()->projections.size()) {
		return true;
	}
	// check if projection topology changed
	for (auto const& [descriptor, projection] : current->projections) {
		auto const& old_projection = old.get_network()->projections.at(descriptor);
		if ((projection.population_pre != old_projection.population_pre) ||
		    (projection.population_post != old_projection.population_post) ||
		    (projection.receptor != old_projection.receptor) ||
		    (projection.connections.size() != old_projection.connections.size())) {
			return true;
		}
		for (size_t i = 0; i < projection.connections.size(); ++i) {
			auto const& connection = projection.connections.at(i);
			auto const& old_connection = old_projection.connections.at(i);
			if ((connection.index_pre != old_connection.index_pre) ||
			    (connection.index_post != old_connection.index_post)) {
				return true;
			}
		}
	}

	auto const current_atomic_network =
	    build_atomic_network(current, old.get_connection_routing_result());
	auto const old_atomic_network =
	    build_atomic_network(old.get_network(), old.get_connection_routing_result());

	return placed_atomic::requires_routing(current_atomic_network, old_atomic_network);
}

} // namespace grenade::vx::network::placed_logical
