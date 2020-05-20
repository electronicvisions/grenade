#include "grenade/vx/event.h"

namespace grenade::vx {

bool TimedSpike::operator==(TimedSpike const& other) const
{
	return time == other.time && payload == other.payload;
}

bool TimedSpike::operator!=(TimedSpike const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx
