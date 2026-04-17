#include "grenade/vx/network/connection_routing_result.h"

#include "hate/indent.h"
#include "hate/join.h"
#include <ostream>

namespace grenade::vx::network {

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
	hate::IndentingOstream ios(os);
	ios << "ConnectionToHardwareRoutes(\n";
	ios << hate::Indentation("\t");
	for (auto const& [receptor_on_compartment, ans] : routes.atomic_neurons_on_target_compartment) {
		ios << receptor_on_compartment << ": [" << hate::join(ans, ", ") << "]\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::network
