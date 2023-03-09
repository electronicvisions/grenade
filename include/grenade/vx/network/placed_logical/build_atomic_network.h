#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_atomic/network.h"
#include "grenade/vx/network/placed_logical/connection_routing_result.h"
#include "grenade/vx/network/placed_logical/network.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

/**
 * Build a hardware network representation for a given network.
 * @param network Network for which to build hardware network representation
 */
std::shared_ptr<placed_atomic::Network> GENPYBIND(visible) build_atomic_network(
    std::shared_ptr<Network> const& network,
    ConnectionRoutingResult const& connection_routing) SYMBOL_VISIBLE;

} // namespace grenade::vx::network::placed_logical
