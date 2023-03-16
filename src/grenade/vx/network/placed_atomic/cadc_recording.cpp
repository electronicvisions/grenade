#include "grenade/vx/network/placed_atomic/cadc_recording.h"

namespace grenade::vx::network::placed_atomic {

CADCRecording::Neuron::Neuron(
    PopulationDescriptor const population, size_t const index, Source const source) :
    population(population), index(index), source(source)
{}

bool CADCRecording::Neuron::operator==(Neuron const& other) const
{
	return population == other.population && index == other.index && source == other.source;
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

} // namespace grenade::vx::network::placed_atomic
