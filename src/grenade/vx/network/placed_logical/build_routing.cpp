#include "grenade/vx/network/placed_logical/build_routing.h"

#include "grenade/vx/network/placed_atomic/routing/routing_builder.h"
#include "grenade/vx/network/placed_logical/build_atomic_network.h"
#include "grenade/vx/network/placed_logical/build_connection_routing.h"
#include "grenade/vx/network/placed_logical/network.h"


namespace grenade::vx::network::placed_logical {

RoutingResult build_routing(
    std::shared_ptr<Network> const& network, std::optional<RoutingOptions> const& options)
{
	assert(network);

	RoutingResult ret;

	ret.connection_routing_result = build_connection_routing(network);

	auto const atomic_network = build_atomic_network(network, ret.connection_routing_result);

	assert(atomic_network);
	placed_atomic::routing::RoutingBuilder builder;
	ret.atomic_routing_result = builder.route(*atomic_network, options);

	return ret;
}

} // namespace grenade::vx::network::placed_logical
