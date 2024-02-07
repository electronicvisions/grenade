#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/signal_flow/data.h"
#include "hate/visibility.h"

namespace grenade::vx::signal_flow GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

/**
 * Pack single spikes to same time into double spikes in-place.
 */
void GENPYBIND(visible) pack_spikes(Data& data) SYMBOL_VISIBLE;

} // namespace grenade::vx::signal_flow
