#include "grenade/vx/network/routing_result.h"

#include "hate/join.h"
#include "hate/timer.h"
#include <ostream>

namespace grenade::vx::network {

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
	for (auto const& [descriptor, projection] : result.connections) {
		os << '\t' << descriptor << '\n';
		size_t i = 0;
		for (auto const& connection : projection) {
			for (auto const& synapse : connection) {
				os << "\t\t" << i << '\n';
				os << "\t\t\t" << synapse << '\n';
			}
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
			os << "\t\t" << hemisphere << ": [" << hate::join_string(label, ", ") << "]\n";
		}
	}

	os << "Internal neuron labels:\n";
	for (auto const& [descriptor, labels] : result.internal_neuron_labels) {
		os << '\t' << descriptor << '\n';
		for (auto const& label : labels) {
			for (auto const& [compartment, l] : label) {
				os << "\t\t" << compartment << ": ";
				for (auto const& ll : l) {
					if (ll) {
						os << *ll << ", ";
					} else {
						os << "disabled, ";
					}
					os << "\n";
				}
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

} // namespace grenade::vx::network
