#include "grenade/vx/network/neuron_recording.h"

namespace grenade::vx::network {

NeuronRecording::Neuron::Neuron(AtomicNeuronOnExecutionInstance const& coordinate, Source source) :
    coordinate(coordinate), source(source)
{}

bool NeuronRecording::Neuron::operator==(Neuron const& other) const
{
	return coordinate == other.coordinate && source == other.source;
}

bool NeuronRecording::Neuron::operator!=(Neuron const& other) const
{
	return !(*this == other);
}


NeuronRecording::NeuronRecording(std::vector<Neuron> const& neurons) : neurons(neurons) {}

bool NeuronRecording::operator==(NeuronRecording const& other) const
{
	return neurons == other.neurons;
}

bool NeuronRecording::operator!=(NeuronRecording const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::network
