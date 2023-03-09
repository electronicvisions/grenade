#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_logical/routing_options.h"
#include "grenade/vx/network/placed_logical/routing_result.h"
#include "hate/visibility.h"
#include <memory>
#include <optional>


namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

struct Network;

/**
 * Route given network.
 * @param network Placed but not routed network to use
 * @return Routing result containing placement and label information for given network
 */
RoutingResult GENPYBIND(visible) build_routing(
    std::shared_ptr<Network> const& network,
    std::optional<RoutingOptions> const& options = std::nullopt) SYMBOL_VISIBLE;

} // namespace grenade::vx::network::placed_logical
