#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_atomic/population.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"

namespace grenade::vx::network::placed_atomic GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_ATOMIC {

/**
 * CADC recording of a collection of neurons.
 */
struct GENPYBIND(visible) CADCRecording
{
	struct Neuron
	{
		PopulationDescriptor population;
		size_t index;
		typedef lola::vx::v3::AtomicNeuron::Readout::Source Source;
		Source source{Source::membrane};

		Neuron() = default;
		Neuron(PopulationDescriptor population, size_t index, Source source) SYMBOL_VISIBLE;

		bool operator==(Neuron const& other) const SYMBOL_VISIBLE;
		bool operator!=(Neuron const& other) const SYMBOL_VISIBLE;
	};
	std::vector<Neuron> neurons{};

	CADCRecording() = default;
	CADCRecording(std::vector<Neuron> const& neurons) SYMBOL_VISIBLE;

	bool operator==(CADCRecording const& other) const SYMBOL_VISIBLE;
	bool operator!=(CADCRecording const& other) const SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network::placed_atomic