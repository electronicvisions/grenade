#include "grenade/vx/network/placed_atomic/routing_result.h"
#include <ostream>

namespace grenade::vx::network::placed_atomic {

std::ostream& operator<<(
    std::ostream& os,
    grenade::vx::network::placed_atomic::RoutingResult::PlacedConnection const& connection)
{
	os << connection.weight << " " << connection.label << " " << connection.synapse_row << " "
	   << connection.synapse_on_row;
	return os;
}

std::ostream& operator<<(
    std::ostream& os, grenade::vx::network::placed_atomic::RoutingResult const& result)
{
	os << "Connections:\n";
	for (auto const& [descriptor, projection] : result.connections) {
		os << '\t' << descriptor << '\n';
		size_t i = 0;
		for (auto const& connection : projection) {
			os << "\t\t" << i << '\n';
			os << "\t\t\t" << connection << '\n';
			i++;
		}
	}

	os << "External spike labels:\n";
	for (auto const& [descriptor, neurons] : result.external_spike_labels) {
		os << '\t' << descriptor << '\n';
		size_t neuron_index = 0;
		for (auto const& neuron : neurons) {
			os << "\t\t" << neuron_index << '\n';
			neuron_index++;
			for (auto const& label : neuron) {
				os << "\t\t\t" << label << '\n';
			}
		}
	}

	os << "Background neuron labels:\n";
	for (auto const& [descriptor, labels] : result.background_spike_source_labels) {
		os << '\t' << descriptor << '\n';
		for (auto const& [hemisphere, label] : labels) {
			os << "\t\t" << hemisphere << ": " << label << '\n';
		}
	}

	os << "Internal neuron labels:\n";
	for (auto const& [descriptor, labels] : result.internal_neuron_labels) {
		os << '\t' << descriptor << '\n';
		for (auto const& label : labels) {
			if (label) {
				os << "\t\t" << *label << '\n';
			} else {
				os << "\t\tdisabled\n";
			}
		}
	}

	os << "SynapseDriver row address compare masks:\n";
	for (auto const& [driver, mask] : result.synapse_driver_compare_masks) {
		os << '\t' << driver << " " << mask << '\n';
	}

	os << "SynapseRow modes (not disabled):\n";
	for (auto const& [row, mode] : result.synapse_row_modes) {
		if (mode != haldls::vx::v3::SynapseDriverConfig::RowMode::disabled) {
			os << '\t' << row << " " << mode << '\n';
		}
	}

	os << "CrossbarNodes:\n";
	for (auto const& [node_coordinate, node] : result.crossbar_nodes) {
		os << '\t' << node_coordinate << " " << node << '\n';
	}

	return os;
}

} // namespace grenade::vx::network::placed_atomic
