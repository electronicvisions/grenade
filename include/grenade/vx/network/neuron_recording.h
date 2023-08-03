#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/atomic_neuron_on_execution_instance.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Neuron recording base class for MADC and CADC recording.
 */
struct GENPYBIND(visible) NeuronRecording
{
	struct Neuron
	{
		AtomicNeuronOnExecutionInstance coordinate{};
		typedef lola::vx::v3::AtomicNeuron::Readout::Source Source;
		Source source{Source::membrane};

		Neuron() = default;
		Neuron(AtomicNeuronOnExecutionInstance const& coordinate, Source source) SYMBOL_VISIBLE;

		bool operator==(Neuron const& other) const SYMBOL_VISIBLE;
		bool operator!=(Neuron const& other) const SYMBOL_VISIBLE;
	};
	std::vector<Neuron> neurons{};

	NeuronRecording() = default;
	NeuronRecording(std::vector<Neuron> const& neurons) SYMBOL_VISIBLE;

	bool operator==(NeuronRecording const& other) const SYMBOL_VISIBLE;
	bool operator!=(NeuronRecording const& other) const SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network
