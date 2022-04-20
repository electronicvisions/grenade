#include "grenade/vx/event.h"

#include "stadls/vx/v3/playback_program_builder.h"

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

std::ostream& operator<<(std::ostream& os, TimedSpike const& spike)
{
	os << "TimedSpike(" << spike.time << ", ";
	std::visit([&os](auto const& p) { os << p; }, spike.payload);
	os << ")";
	return os;
}

} // namespace grenade::vx
