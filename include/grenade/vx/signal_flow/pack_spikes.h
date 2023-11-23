#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/signal_flow/io_data_map.h"
#include "hate/visibility.h"

namespace grenade::vx::signal_flow GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

/**
 * Pack single spikes to same time into double spikes in-place.
 */
void GENPYBIND(visible) pack_spikes(IODataMap& data) SYMBOL_VISIBLE;

} // namespace grenade::vx::signal_flow
