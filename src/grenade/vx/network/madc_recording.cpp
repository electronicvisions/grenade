#include "grenade/vx/network/madc_recording.h"

namespace grenade::vx::network {

MADCRecording::Neuron::Neuron(AtomicNeuronOnNetwork const& coordinate, Source source) :
    coordinate(coordinate), source(source)
{}

bool MADCRecording::Neuron::operator==(Neuron const& other) const
{
	return coordinate == other.coordinate && source == other.source;
}

bool MADCRecording::Neuron::operator!=(Neuron const& other) const
{
	return !(*this == other);
}


MADCRecording::MADCRecording(std::vector<Neuron> const& neurons) : neurons(neurons) {}

bool MADCRecording::operator==(MADCRecording const& other) const
{
	return neurons == other.neurons;
}

bool MADCRecording::operator!=(MADCRecording const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::network
