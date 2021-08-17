#include "grenade/vx/logical_network/madc_recording.h"

namespace grenade::vx::logical_network {

bool MADCRecording::operator==(MADCRecording const& other) const
{
	return population == other.population && index == other.index && source == other.source;
}

bool MADCRecording::operator!=(MADCRecording const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::logical_network
