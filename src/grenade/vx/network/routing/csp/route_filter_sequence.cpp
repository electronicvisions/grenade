#include "grenade/vx/network/routing/csp/route_filter_sequence.h"

#include "hate/join.h"
#include <ostream>

namespace grenade::vx::network::routing::csp {

RouteFilterSequence::RouteFilterSequence(Descriptors descriptors) :
    descriptors(std::move(descriptors))
{
}

bool RouteFilterSequence::operator==(RouteFilterSequence const& other) const
{
	return descriptors == other.descriptors;
}

std::ostream& operator<<(std::ostream& os, RouteFilterSequence const& value)
{
	return os << "RouteFilterSequence(" << hate::join_string(value.descriptors, ", ") << ")";
}

} // namespace grenade::vx::network::routing::csp