#include "grenade/vx/vertex/crossbar_l2_output.h"

#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/crossbar_node.h"
#include "halco/hicann-dls/vx/v1/padi.h"

#include <stdexcept>

namespace grenade::vx::vertex {

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

} // namespace grenade::vx::vertex
