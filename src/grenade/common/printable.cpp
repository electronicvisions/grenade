#include "grenade/common/printable.h"

#include <ostream>

namespace grenade::common {

Printable::~Printable() {}

std::ostream& operator<<(std::ostream& os, Printable const& printable)
{
	return printable.print(os);
}

} // namespace grenade::common
