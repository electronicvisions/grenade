#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_atomic/projection.h"
#include "grenade/vx/network/placed_logical/projection.h"
#include <vector>

namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

/**
 * Split weight of connection into single hardware synapse circuit weight values.
 * This function splits like 123 -> 63 + 60 + 0.
 * @param weight Weight to split
 * @param num Number of synapses to split weight onto
 */
std::vector<placed_atomic::Projection::Connection::Weight> GENPYBIND(visible)
    build_connection_weight_split(Projection::Connection::Weight const& weight, size_t num)
        SYMBOL_VISIBLE;

} // namespace grenade::vx::network::placed_logical
