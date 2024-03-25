#include "grenade/vx/network/abstract/printable.h"

#include <ostream>

namespace grenade::vx::network {

Printable::~Printable() {}

std::ostream& operator<<(std::ostream& os, Printable const& printable)
{
	return printable.print(os);
}

} // namespace grenade::vx::network
