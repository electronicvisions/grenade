#include "grenade/vx/network/placed_logical/routing_options.h"

#include "hate/timer.h"
#include <ostream>

namespace grenade::vx::network::placed_logical {

RoutingOptions::RoutingOptions() {}

std::ostream& operator<<(std::ostream& os, RoutingOptions const& options)
{
	os << "RoutingOptions(\n";
	os << "\tsynapse_driver_allocation_policy: " << options.synapse_driver_allocation_policy
	   << "\n";
	os << "\tsynapse_driver_allocation_timeout: "
	   << (options.synapse_driver_allocation_timeout
	           ? hate::to_string(*options.synapse_driver_allocation_timeout)
	           : "infinite");
	os << "\n)";
	return os;
}

} // namespace grenade::vx::network::placed_logical
