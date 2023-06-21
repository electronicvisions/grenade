#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/population_descriptor.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * CADC recording of a collection of neurons.
 */
struct GENPYBIND(visible) CADCRecording
{
	struct Neuron
	{
		PopulationDescriptor population{};
		size_t neuron_on_population{0};
		halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron compartment_on_neuron{};
		size_t atomic_neuron_on_compartment{0};
		typedef lola::vx::v3::AtomicNeuron::Readout::Source Source;
		Source source{Source::membrane};

		Neuron() = default;
		Neuron(
		    PopulationDescriptor population,
		    size_t neuron_on_population,
		    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron compartment_on_neuron,
		    size_t atomic_neuron_on_compartment,
		    Source source) SYMBOL_VISIBLE;

		bool operator==(Neuron const& other) const SYMBOL_VISIBLE;
		bool operator!=(Neuron const& other) const SYMBOL_VISIBLE;
	};
	std::vector<Neuron> neurons{};

	CADCRecording() = default;
	CADCRecording(std::vector<Neuron> const& neurons) SYMBOL_VISIBLE;

	bool operator==(CADCRecording const& other) const SYMBOL_VISIBLE;
	bool operator!=(CADCRecording const& other) const SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network
