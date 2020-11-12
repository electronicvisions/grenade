#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/population.h"
#include "lola/vx/v2/neuron.h"

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace network {

/**
 * MADC recording of a single neuron.
 */
struct GENPYBIND(visible) MADCRecording
{
	PopulationDescriptor population{};
	size_t index{0};
	lola::vx::v2::AtomicNeuron::Readout::Source source{
	    lola::vx::v2::AtomicNeuron::Readout::Source::membrane};

	MADCRecording() = default;
};

} // namespace network

} // namespace grenade::vx
