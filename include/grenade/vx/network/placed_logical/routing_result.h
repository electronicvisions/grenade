#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_atomic/routing_result.h"
#include "grenade/vx/network/placed_logical/connection_routing_result.h"

namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

/**
 * Result of routing.
 */
struct GENPYBIND(visible) RoutingResult
{
	ConnectionRoutingResult connection_routing_result;

	placed_atomic::RoutingResult atomic_routing_result;

	RoutingResult() = default;
};

} // namespace grenade::vx::network::placed_logical
