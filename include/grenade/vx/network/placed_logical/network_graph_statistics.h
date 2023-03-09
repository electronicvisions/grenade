#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_atomic/network_graph_statistics.h"
#include "hate/visibility.h"


namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

struct NetworkGraph;
typedef placed_atomic::NetworkGraphStatistics NetworkGraphStatistics GENPYBIND(visible);

/**
 * Extract statistics from network graph.
 */
NetworkGraphStatistics GENPYBIND(visible)
    extract_statistics(NetworkGraph const& network_graph) SYMBOL_VISIBLE;

} // namespace grenade::vx::network::placed_logical
