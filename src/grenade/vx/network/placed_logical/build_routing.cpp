#include "grenade/vx/network/placed_logical/build_routing.h"

#include "grenade/vx/network/placed_logical/build_atomic_network.h"
#include "grenade/vx/network/placed_logical/build_connection_routing.h"
#include "grenade/vx/network/placed_logical/network.h"
#include "grenade/vx/network/placed_logical/routing/routing_builder.h"


namespace grenade::vx::network::placed_logical {

RoutingResult build_routing(
    std::shared_ptr<Network> const& network, std::optional<RoutingOptions> const& options)
{
	assert(network);
	auto const connection_routing_result = build_connection_routing(network);
	routing::RoutingBuilder builder;
	return builder.route(*network, connection_routing_result, options);
}

} // namespace grenade::vx::network::placed_logical
