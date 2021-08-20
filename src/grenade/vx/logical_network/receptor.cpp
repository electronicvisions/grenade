#include "grenade/vx/logical_network/receptor.h"

#include <ostream>

namespace grenade::vx::logical_network {

Receptor::Receptor(ID const id, Type const type) : id(id), type(type) {}

bool Receptor::operator==(Receptor const& other) const
{
	return id == other.id && type == other.type;
}

bool Receptor::operator!=(Receptor const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, Receptor const& receptor)
{
	os << "Receptor(" << receptor.id << ", " << receptor.type << ")";
	return os;
}

} // namespace grenade::vx::logical_network
