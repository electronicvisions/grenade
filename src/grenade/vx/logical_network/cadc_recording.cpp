#include "grenade/vx/logical_network/cadc_recording.h"

namespace grenade::vx::logical_network {

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

} // namespace grenade::vx::logical_network
