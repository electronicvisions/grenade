#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_atomic/routing_options.h"

namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

/**
 * Options to be passed to routing algorithm.
 */
typedef placed_atomic::RoutingOptions RoutingOptions GENPYBIND(visible);

} // namespace grenade::vx::network::placed_logical
