#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"

#include "grenade/vx/signal_flow/port_restriction.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include <ostream>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

std::ostream& operator<<(std::ostream& os, CrossbarL2Output const&)
{
	os << "CrossbarL2Output()";
	return os;
}

bool CrossbarL2Output::supports_input_from(
    CrossbarNode const& input, std::optional<PortRestriction> const& restriction) const
{
	if (restriction && !restriction->is_restriction_of(input.output())) {
		throw std::runtime_error(
		    "Given restriction is not a restriction of input vertex output port.");
	}
	return static_cast<bool>(
	    input.get_coordinate().toCrossbarOutputOnDLS().toCrossbarL2OutputOnDLS());
}

bool CrossbarL2Output::operator==(CrossbarL2Output const&) const
{
	return true;
}

bool CrossbarL2Output::operator!=(CrossbarL2Output const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::signal_flow::vertex
