#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/neuron_recording.h"
#include "halco/hicann-dls/vx/v3/readout.h"
#include "hate/visibility.h"
#include <map>

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Pad recording of neuron source(s).
 * This recording only forwards signals to the analog readout pad(s), no recorded data is available
 * after execution.
 */
struct GENPYBIND(visible) PadRecording
{
	PadRecording() = default;

	struct Source
	{
		NeuronRecording::Neuron neuron{};
		bool enable_buffered{false};

		Source() = default;
		Source(NeuronRecording::Neuron const& neuron, bool enable_buffered) SYMBOL_VISIBLE;

		bool operator==(Source const& other) const SYMBOL_VISIBLE;
		bool operator!=(Source const& other) const SYMBOL_VISIBLE;
	};

	typedef std::map<halco::hicann_dls::vx::v3::PadOnDLS, Source> Recordings;
	Recordings recordings;

	/**
	 * Construct pad recording.
	 * @param recordings Recordings to perform
	 */
	PadRecording(Recordings const& recordings) SYMBOL_VISIBLE;

	bool operator==(PadRecording const& other) const SYMBOL_VISIBLE;
	bool operator!=(PadRecording const& other) const SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network
