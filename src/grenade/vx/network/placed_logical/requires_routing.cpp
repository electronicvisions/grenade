#include "grenade/vx/network/placed_logical/requires_routing.h"

#include "grenade/vx/network/placed_atomic/requires_routing.h"
#include "grenade/vx/network/placed_logical/build_atomic_network.h"
#include "grenade/vx/network/placed_logical/build_connection_routing.h"
#include "grenade/vx/network/placed_logical/network.h"

namespace grenade::vx::network::placed_logical {

bool requires_routing(std::shared_ptr<Network> const& current, std::shared_ptr<Network> const& old)
{
	assert(current);
	assert(old);

	auto const current_atomic_network =
	    build_atomic_network(current, build_connection_routing(current));
	auto const old_atomic_network = build_atomic_network(old, build_connection_routing(old));

	return placed_atomic::requires_routing(current_atomic_network, old_atomic_network);
}

} // namespace grenade::vx::network::placed_logical
