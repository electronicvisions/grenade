#include "grenade/vx/signal_flow/port.h"

#include <ostream>

namespace grenade::vx::signal_flow {

std::ostream& operator<<(std::ostream& os, Port const& port)
{
	return (os << "Port(" << port.size << ", " << port.type << ")");
}

} // namespace grenade::vx::signal_flow
