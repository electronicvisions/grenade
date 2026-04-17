#pragma once
#include "grenade/common/linked_topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/connection_routing_result.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Build a connection routing for a given network.
 * This is the first step in two-stage routing towards a hardware data-flow graph representation.
 * @param network Network execution instance for which to build connection routing
 */
ConnectionRoutingResult GENPYBIND(visible) build_connection_routing(
    grenade::common::LinkedTopology const& topology,
    std::vector<grenade::common::VertexOnTopology> const& local_vertex_descriptors) SYMBOL_VISIBLE;

} // namespace network
} // namespace grenade::vx
