#include "grenade/vx/network/routing/csp/source_target_pair.h"
#include "hate/indent.h"

namespace grenade::vx::network::routing::csp {

SourceTargetPair::SourceTargetPair(
    grenade::common::VertexOnTopology source, grenade::common::VertexOnTopology target) :
    source(source), target(target)
{
}

std::ostream& operator<<(std::ostream& os, SourceTargetPair const& value)
{
	hate::IndentingOstream ios(os);
	ios << "SourceTargetPair(\n";

	ios << hate::Indentation("\t");
	ios << "source: " << value.source << "\n";
	ios << "target:" << value.target << "\n";

	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::network::routing::csp