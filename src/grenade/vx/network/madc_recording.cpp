#include "grenade/vx/network/madc_recording.h"

namespace grenade::vx::network {

bool MADCRecording::operator==(MADCRecording const& other) const
{
	return population == other.population && index == other.index && source == other.source;
}

bool MADCRecording::operator!=(MADCRecording const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::network
