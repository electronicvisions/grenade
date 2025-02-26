#pragma once
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <memory>


namespace grenade::vx { namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct Network;
struct NetworkGraph;

/**
 * Get whether the current network requires routing compared to the old network.
 * @param current Current network
 * @param old_graph Old routed network
 * @return Boolean value
 */
bool GENPYBIND(visible) requires_routing(
    std::shared_ptr<Network> const& current, NetworkGraph const& old_graph) SYMBOL_VISIBLE;

} // namespace network
} // namespace grenade::vx
