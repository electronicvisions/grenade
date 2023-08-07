#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/neuron_recording.h"
#include "hate/visibility.h"

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * MADC recording of a single neuron.
 */
struct GENPYBIND(visible) MADCRecording : public NeuronRecording
{
	MADCRecording() = default;
	MADCRecording(std::vector<Neuron> const& neurons) SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network
