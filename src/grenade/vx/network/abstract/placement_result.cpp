#include "grenade/vx/network/abstract/placement_result.h"

#include "hate/indent.h"
#include "hate/timer.h"
#include <ostream>

namespace grenade::vx::network::abstract {

std::ostream& operator<<(std::ostream& os, PlacementResult::TimingStatistics const& result)
{
	return os << "TimingStatistics(" << hate::to_string(result.placement) << ")";
}

std::ostream& operator<<(std::ostream& os, PlacementResult const& result)
{
	hate::IndentingOstream ios(os);
	ios << "PlacementResult(\n";

	for (auto const& [vertex_descriptor, anchors] : result.neuron_anchors) {
		ios << hate::Indentation("\t");
		ios << vertex_descriptor << ":\n";
		ios << hate::Indentation("\t\t");
		for (auto const& anchor : anchors) {
			ios << anchor << "\n";
		}
	}

	for (auto const& [vertex_descriptor, background] : result.background_locations) {
		ios << hate::Indentation("\t");
		ios << vertex_descriptor << ":\n";
		ios << hate::Indentation("\t\t");
		for (auto const& [hemisphere, padi_bus] : background) {
			ios << hemisphere << ": " << padi_bus << "\n";
		}
	}

	ios << hate::Indentation("\t");
	ios << result.timing_statistics << "\n";

	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::network::abstract
