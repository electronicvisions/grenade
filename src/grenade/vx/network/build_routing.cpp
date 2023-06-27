#include "grenade/vx/network/build_routing.h"

#include "grenade/vx/network/build_connection_routing.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/routing/greedy/routing_builder.h"


namespace grenade::vx::network {

RoutingResult build_routing(
    std::shared_ptr<Network> const& network, std::optional<RoutingOptions> const& options)
{
	assert(network);
	auto const connection_routing_result = build_connection_routing(network);
	routing::greedy::RoutingBuilder builder;
	return builder.route(*network, connection_routing_result, options);
}

} // namespace grenade::vx::network
