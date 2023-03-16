#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_logical/network.h"
#include "grenade/vx/network/placed_logical/network_graph.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

/**
 * Build a hardware network representation for a given network.
 * @param network Network for which to build hardware network representation
 */
NetworkGraph GENPYBIND(visible)
    build_network_graph(std::shared_ptr<Network> const& network) SYMBOL_VISIBLE;

} // namespace grenade::vx::network::placed_logical
