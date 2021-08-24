#include "grenade/vx/logical_network/extract_output.h"

#include "grenade/vx/network/extract_output.h"

namespace grenade::vx::logical_network {

std::vector<std::map<
    std::tuple<PopulationDescriptor, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
    std::vector<haldls::vx::v3::ChipTime>>>
extract_neuron_spikes(
    IODataMap const& data,
    NetworkGraph const& network_graph,
    network::NetworkGraph const& hardware_network_graph)
{
	if (network_graph.get_hardware_network() != hardware_network_graph.get_network()) {
		throw std::runtime_error(
		    "Logical and hardware network graph need to feature matching hardware network.");
	}
	assert(network_graph.get_network());

	std::map<
	    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS,
	    std::tuple<
	        PopulationDescriptor, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>>
	    spike_master_lookup;
	for (auto const& [descriptor, population] : network_graph.get_network()->populations) {
		if (!std::holds_alternative<Population>(population)) {
			continue;
		}
		size_t i = 0;
		for (auto const& neuron : std::get<Population>(population).neurons) {
			for (auto const& [compartment_on_neuron, compartment] : neuron.compartments) {
				if (compartment.spike_master && compartment.spike_master->enable_record_spikes) {
					spike_master_lookup[neuron.coordinate.get_placed_compartments()
					                        .at(compartment_on_neuron)
					                        .at(compartment.spike_master->neuron_on_compartment)] =
					    std::tuple{descriptor, i, compartment_on_neuron};
				}
				i++;
			}
		}
	}

	auto const atomic_neuron_events = network::extract_neuron_spikes(data, hardware_network_graph);

	std::vector<std::map<
	    std::tuple<
	        PopulationDescriptor, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
	    std::vector<haldls::vx::v3::ChipTime>>>
	    logical_neuron_events(atomic_neuron_events.size());

	for (size_t batch = 0; batch < logical_neuron_events.size(); ++batch) {
		auto& local_logical_neuron_events = logical_neuron_events.at(batch);
		auto const& local_atomic_neuron_events = atomic_neuron_events.at(batch);

		for (auto const& [atomic_neuron, times] : local_atomic_neuron_events) {
			if (spike_master_lookup.contains(atomic_neuron)) {
				local_logical_neuron_events[spike_master_lookup.at(atomic_neuron)] = times;
			}
		}
	}

	return logical_neuron_events;
}

} // namespace grenade::vx::logical_network
