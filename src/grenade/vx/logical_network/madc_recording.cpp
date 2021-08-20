#include "grenade/vx/logical_network/madc_recording.h"

namespace grenade::vx::logical_network {

bool MADCRecording::operator==(MADCRecording const& other) const
{
	return population == other.population && neuron_on_population == other.neuron_on_population &&
	       compartment_on_neuron == other.compartment_on_neuron &&
	       atomic_neuron_on_compartment == other.atomic_neuron_on_compartment &&
	       source == other.source;
}

bool MADCRecording::operator!=(MADCRecording const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::logical_network
