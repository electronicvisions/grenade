#include "grenade/vx/network/placed_logical/connection_routing_result.h"

#include "hate/join.h"
#include <ostream>

namespace grenade::vx::network::placed_logical {

bool ConnectionToHardwareRoutes::operator==(ConnectionToHardwareRoutes const& other) const
{
	return atomic_neurons_on_target_compartment == other.atomic_neurons_on_target_compartment;
}

bool ConnectionToHardwareRoutes::operator!=(ConnectionToHardwareRoutes const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, ConnectionToHardwareRoutes const& routes)
{
	os << "ConnectionToHardwareRoutes("
	   << hate::join_string(routes.atomic_neurons_on_target_compartment, ", ") << ")";
	return os;
}

} // namespace grenade::vx::network::placed_logical
