#include "grenade/vx/port.h"

#include <ostream>

namespace grenade::vx {

std::ostream& operator<<(std::ostream& os, Port const& port)
{
	return (os << "Port(" << port.size << ", " << port.type << ")");
}

} // namespace grenade::vx
