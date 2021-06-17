#include "grenade/vx/event.h"

#include "stadls/vx/v2/playback_program_builder.h"

namespace grenade::vx {

TimedSpike::TimedSpike(Time const& time, Payload const& payload) : time(time), payload(payload) {}

bool TimedSpike::operator==(TimedSpike const& other) const
{
	return time == other.time && payload == other.payload;
}

bool TimedSpike::operator!=(TimedSpike const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx
