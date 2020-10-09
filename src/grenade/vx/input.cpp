#include "grenade/vx/input.h"

#include <ostream>

namespace grenade::vx {

bool Input::operator==(Input const& other) const
{
	return descriptor == other.descriptor && port_restriction == other.port_restriction;
}

bool Input::operator!=(Input const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, Input const& data)
{
	os << "Input(descriptor: " << data.descriptor;
	if (data.port_restriction) {
		os << ", " << *data.port_restriction;
	}
	os << ")";
	return os;
}

} // namespace grenade::vx
