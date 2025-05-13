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


NeuronRecording::NeuronRecording(
    std::vector<Neuron> const& neurons,
    common::EntityOnChip::ChipOnExecutor const chip_on_executor) :
    common::EntityOnChip(chip_on_executor), neurons(neurons)
{}

bool NeuronRecording::operator==(NeuronRecording const& other) const
{
	return neurons == other.neurons && (static_cast<common::EntityOnChip const&>(*this) ==
	                                    static_cast<common::EntityOnChip const&>(other));
}

bool NeuronRecording::operator!=(NeuronRecording const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::network
