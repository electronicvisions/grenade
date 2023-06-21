#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/atomic_neuron_on_network.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * MADC recording of a single neuron.
 */
struct GENPYBIND(visible) MADCRecording
{
	struct Neuron
	{
		AtomicNeuronOnNetwork coordinate{};
		typedef lola::vx::v3::AtomicNeuron::Readout::Source Source;
		Source source{Source::membrane};

		Neuron() = default;
		Neuron(AtomicNeuronOnNetwork const& coordinate, Source source) SYMBOL_VISIBLE;

		bool operator==(Neuron const& other) const SYMBOL_VISIBLE;
		bool operator!=(Neuron const& other) const SYMBOL_VISIBLE;
	};
	std::vector<Neuron> neurons{};

	MADCRecording() = default;
	MADCRecording(std::vector<Neuron> const& neurons) SYMBOL_VISIBLE;

	bool operator==(MADCRecording const& other) const SYMBOL_VISIBLE;
	bool operator!=(MADCRecording const& other) const SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network
