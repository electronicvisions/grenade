#include "grenade/vx/signal_flow/vertex/crossbar_l2_input.h"

#include "grenade/vx/signal_flow/vertex/external_input.h"

#include <ostream>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

std::ostream& operator<<(std::ostream& os, CrossbarL2Input const&)
{
	os << "CrossbarL2Input()";
	return os;
}

bool CrossbarL2Input::operator==(CrossbarL2Input const&) const
{
	return true;
}

bool CrossbarL2Input::operator!=(CrossbarL2Input const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::signal_flow::vertex
