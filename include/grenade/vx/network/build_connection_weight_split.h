#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/projection.h"
#include "lola/vx/v3/synapse.h"
#include <vector>

namespace grenade::vx { namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Split weight of connection into single hardware synapse circuit weight values.
 * This function splits like 123 -> 63 + 60 + 0.
 * @param weight Weight to split
 * @param num Number of synapses to split weight onto
 */
std::vector<lola::vx::v3::SynapseMatrix::Weight> GENPYBIND(visible) build_connection_weight_split(
    Projection::Connection::Weight const& weight, size_t num) SYMBOL_VISIBLE;

} // namespace network
} // namespace grenade::vx
