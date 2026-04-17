#include "grenade/vx/network/receptor.h"

#include <ostream>

namespace grenade::vx::network {

std::ostream& operator<<(std::ostream& os, Receptor::Type const& receptor_type)
{
	return os << (receptor_type == Receptor::Type::excitatory ? "excitatory" : "inhibitory");
}

} // namespace grenade::vx::network
