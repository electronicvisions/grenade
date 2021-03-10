#include "grenade/vx/network/routing_result.h"
#include <ostream>

namespace grenade::vx {

namespace network {

std::ostream& operator<<(
    std::ostream& os, grenade::vx::network::RoutingResult::PlacedConnection const& connection)
{
	os << connection.weight << " " << connection.label << " " << connection.synapse_row << " "
	   << connection.synapse_on_row;
	return os;
}

std::ostream& operator<<(std::ostream& os, grenade::vx::network::RoutingResult const& result)
{
	os << "Connections:\n";
	for (auto const& [descriptor, projections] : result.connections) {
		os << descriptor << '\n';
		for (auto const& projection : projections) {
			os << '\t' << projection << '\n';
		}
	}

	os << "External spike labels:\n";
	for (auto const& [descriptor, neurons] : result.external_spike_labels) {
		os << descriptor << '\n';
		size_t neuron_index = 0;
		for (auto const& neuron : neurons) {
			os << '\t' << neuron_index << '\n';
			neuron_index++;
			for (auto const& label : neuron) {
				os << "\t\t" << label << '\n';
			}
		}
	}

	os << "Internal neuron labels:\n";
	for (auto const& [descriptor, labels] : result.internal_neuron_labels) {
		os << descriptor << '\n';
		for (auto const& label : labels) {
			os << '\t' << label << '\n';
		}
	}

	os << "SynapseDriver row address compare masks:\n";
	for (auto const& [driver, mask] : result.synapse_driver_compare_masks) {
		os << driver << " " << mask << '\n';
	}

	os << "SynapseRow modes (not disabled):\n";
	for (auto const& [row, mode] : result.synapse_row_modes) {
		if (mode != haldls::vx::v2::SynapseDriverConfig::RowMode::disabled) {
			os << row << " " << mode << '\n';
		}
	}

	os << "CrossbarNodes:\n";
	for (auto const& [node_coordinate, node] : result.crossbar_nodes) {
		os << node_coordinate << " " << node << '\n';
	}

	return os;
}

} // namespace network

} // namespace grenade::vx
