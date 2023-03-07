#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_logical/connection_routing_result.h"
#include "grenade/vx/network/placed_logical/network.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

/**
 * Build a connection routing for a given network.
 * This is the first step in two-stage routing towards a hardware data-flow graph representation.
 * @param network Network for which to build connection routing
 */
ConnectionRoutingResult GENPYBIND(visible)
    build_connection_routing(std::shared_ptr<Network> const& network) SYMBOL_VISIBLE;

} // namespace grenade::vx::network::placed_logical
