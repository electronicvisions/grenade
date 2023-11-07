#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/neuron_recording.h"
#include "hate/visibility.h"

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * CADC recording of a collection of neurons.
 */
struct GENPYBIND(visible) CADCRecording : public NeuronRecording
{
	CADCRecording() = default;
	CADCRecording(
	    std::vector<Neuron> const& neurons,
	    common::EntityOnChip::ChipCoordinate chip_coordinate =
	        common::EntityOnChip::ChipCoordinate()) SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network
