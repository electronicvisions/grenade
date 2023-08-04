#include "grenade/vx/network/cadc_recording.h"

namespace grenade::vx::network {

CADCRecording::Neuron::Neuron(
    PopulationOnNetwork population,
    size_t neuron_on_population,
    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron compartment_on_neuron,
    size_t atomic_neuron_on_compartment,
    Source source) :
    population(population),
    neuron_on_population(neuron_on_population),
    compartment_on_neuron(compartment_on_neuron),
    atomic_neuron_on_compartment(atomic_neuron_on_compartment),
    source(source)
{}

bool CADCRecording::Neuron::operator==(Neuron const& other) const
{
	return population == other.population && neuron_on_population == other.neuron_on_population &&
	       compartment_on_neuron == other.compartment_on_neuron &&
	       atomic_neuron_on_compartment == other.atomic_neuron_on_compartment &&
	       source == other.source;
}

bool CADCRecording::Neuron::operator!=(Neuron const& other) const
{
	return !(*this == other);
}


CADCRecording::CADCRecording(std::vector<Neuron> const& neurons) : neurons(neurons) {}

bool CADCRecording::operator==(CADCRecording const& other) const
{
	return neurons == other.neurons;
}

bool CADCRecording::operator!=(CADCRecording const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::network
