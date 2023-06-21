#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/connection_routing_result.h"
#include "grenade/vx/network/network.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Build a connection routing for a given network.
 * This is the first step in two-stage routing towards a hardware data-flow graph representation.
 * @param network Network for which to build connection routing
 */
ConnectionRoutingResult GENPYBIND(visible)
    build_connection_routing(std::shared_ptr<Network> const& network) SYMBOL_VISIBLE;

} // namespace grenade::vx::network
