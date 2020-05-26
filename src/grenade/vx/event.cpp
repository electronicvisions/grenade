#include "grenade/vx/event.h"

namespace grenade::vx {

bool TimedSpikeEvent::operator==(TimedSpikeEvent const& other) const
{
	return time == other.time && payload == other.payload;
}

bool TimedSpikeEvent::operator!=(TimedSpikeEvent const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx
