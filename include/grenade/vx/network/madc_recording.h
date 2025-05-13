#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/neuron_recording.h"
#include "hate/visibility.h"

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * MADC recording of a single neuron.
 */
struct GENPYBIND(visible) MADCRecording : public NeuronRecording
{
	MADCRecording() = default;
	MADCRecording(
	    std::vector<Neuron> const& neurons,
	    common::EntityOnChip::ChipOnExecutor chip_on_executor =
	        common::EntityOnChip::ChipOnExecutor()) SYMBOL_VISIBLE;
};

} // namespace network
} // namespace grenade::vx
