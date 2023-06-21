#include "grenade/vx/signal_flow/event.h"

#include <ostream>

namespace grenade::vx::signal_flow {

bool MADCSampleFromChip::operator==(MADCSampleFromChip const& other) const
{
	return value == other.value && channel == other.channel;
}

bool MADCSampleFromChip::operator!=(MADCSampleFromChip const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, MADCSampleFromChip const& sample)
{
	return os << "MADCSampleFromChip(" << sample.value << ", " << sample.channel << ")";
}

} // namespace grenade::vx::signal_flow
