#include "grenade/vx/network/network.h"

namespace grenade::vx::network {

bool Network::operator==(Network const& other) const
{
	return populations == other.populations && projections == other.projections &&
	       madc_recording == other.madc_recording;
}

bool Network::operator!=(Network const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::network
