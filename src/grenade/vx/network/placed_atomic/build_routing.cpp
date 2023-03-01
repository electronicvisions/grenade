#include "grenade/vx/network/placed_atomic/build_routing.h"

#include "grenade/vx/network/placed_atomic/network.h"
#include "grenade/vx/network/placed_atomic/routing/routing_builder.h"


namespace grenade::vx::network::placed_atomic {

RoutingResult build_routing(
    std::shared_ptr<Network> const& network, std::optional<RoutingOptions> const& options)
{
	assert(network);
	routing::RoutingBuilder builder;
	return builder.route(*network, options);
}

} // namespace grenade::vx::network::placed_atomic
