#pragma once
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <memory>


namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

struct Network;
struct NetworkGraph;

/**
 * Get whether the current network requires routing compared to the old network.
 * @param current Current network
 * @param old Old network graph
 * @return Boolean value
 */
bool GENPYBIND(visible) requires_routing(
    std::shared_ptr<Network> const& current, NetworkGraph const& old) SYMBOL_VISIBLE;

} // namespace grenade::vx::network::placed_logical