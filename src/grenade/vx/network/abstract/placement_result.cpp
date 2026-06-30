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

	for (auto const& [execution_instance, execution_instance_result] : result.chips) {
		ios << hate::Indentation("\t");
		ios << execution_instance;
		for (auto const& [chip, chip_result] : execution_instance_result) {
			ios << hate::Indentation("\t\t");
			ios << chip;

			for (auto const& [vertex_descriptor, neuron_anchor_mapping] :
			     chip_result.neuron_anchors) {
				ios << hate::Indentation("\t\t\t");
				ios << vertex_descriptor << ":\n";
				ios << hate::Indentation("\t\t\t\t");
				for (auto const& [neuron_index, anchor] : neuron_anchor_mapping) {
					ios << "neuron_index: " << neuron_index << ", anchor: " << anchor << "\n";
				}
			}

			for (auto const& [vertex_descriptor, background] : chip_result.background_locations) {
				ios << hate::Indentation("\t\t\t");
				ios << vertex_descriptor << ":\n";
				ios << hate::Indentation("\t\t\t\t");
				for (auto const& [hemisphere, padi_bus] : background) {
					ios << hemisphere << ": " << padi_bus << "\n";
				}
			}
		}
	}

	ios << hate::Indentation("\t");
	ios << result.timing_statistics << "\n";

	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::network::abstract
