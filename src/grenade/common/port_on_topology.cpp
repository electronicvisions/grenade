#include "grenade/common/port_on_topology.h"

namespace std {

std::ostream& operator<<(std::ostream& os, grenade::common::PortOnTopology const& value)
{
	return os << "PortOnTopology(" << value.first << ", " << value.second << ")";
}

} // namespace std
