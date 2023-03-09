#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_logical/network.h"
#include "grenade/vx/network/placed_logical/network_graph.h"
#include "grenade/vx/network/placed_logical/routing_result.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

/**
 * Build a hardware network representation for a given network.
 * @param network Network for which to build hardware network representation
 * @param routing_result Routing result to use to build hardware network representation
 */
NetworkGraph GENPYBIND(visible) build_network_graph(
    std::shared_ptr<Network> const& network, RoutingResult const& routing_result) SYMBOL_VISIBLE;


/**
 * Update an exisiting hardware graph representation.
 * For this to work, no new routing has to have been required.
 * @param network_graph Existing hardware graph representation to update or fill with newly built
 * instance
 * @param network New network for which to update or build
 */
void GENPYBIND(visible) update_network_graph(
    NetworkGraph& network_graph, std::shared_ptr<Network> const& network) SYMBOL_VISIBLE;

} // namespace grenade::vx::network::placed_logical
