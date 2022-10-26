#include "grenade/vx/logical_network/extract_output.h"

#include "grenade/vx/network/extract_output.h"
#include "hate/variant.h"

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


namespace {

template <typename T>
void extract_plasticity_rule_recording_data_per_synapse(
    std::map<ProjectionDescriptor, PlasticityRule::TimedRecordingData::EntryPerSynapse>&
        local_logical_data,
    size_t const num_periods,
    ProjectionDescriptor const& descriptor,
    NetworkGraph const& network_graph,
    size_t const batch_size,
    size_t const hardware_index,
    size_t const index,
    std::vector<TimedDataSequence<std::vector<T>>> const& dd)
{
	if (!local_logical_data.contains(descriptor)) {
		std::vector<TimedDataSequence<std::vector<std::vector<T>>>> logical_dd(batch_size);
		for (auto& batch : logical_dd) {
			batch.resize(num_periods);
			for (auto& sample : batch) {
				sample.data.resize(
				    network_graph.get_network()->projections.at(descriptor).connections.size());
			}
			for (size_t b = 0; b < dd.size(); ++b) {
				for (size_t s = 0; s < dd.at(b).size(); ++s) {
					auto& local_logical_dd = logical_dd.at(b).at(s);
					auto& local_dd = dd.at(b).at(s);
					local_logical_dd.chip_time = local_dd.chip_time;
					local_logical_dd.fpga_time = local_dd.fpga_time;
				}
			}
		}
		local_logical_data[descriptor] = logical_dd;
	}
	auto& logical_dd = std::get<std::vector<TimedDataSequence<std::vector<std::vector<T>>>>>(
	    local_logical_data.at(descriptor));
	for (size_t b = 0; b < dd.size(); ++b) {
		for (size_t s = 0; s < dd.at(b).size(); ++s) {
			auto& local_logical_dd = logical_dd.at(b).at(s);
			auto& local_dd = dd.at(b).at(s);
			local_logical_dd.data.at(index).push_back(local_dd.data.at(hardware_index));
		}
	}
}

} // namespace

PlasticityRule::RecordingData extract_plasticity_rule_recording_data(
    IODataMap const& data,
    NetworkGraph const& network_graph,
    network::NetworkGraph const& hardware_network_graph,
    PlasticityRuleDescriptor descriptor)
{
	auto const hardware_data = network::extract_plasticity_rule_recording_data(
	    data, hardware_network_graph,
	    network_graph.get_plasticity_rule_translation().at(descriptor));

	auto const batch_size = data.batch_size();

	return std::visit(
	    hate::overloaded{
	        [](network::PlasticityRule::RawRecordingData const& data)
	            -> PlasticityRule::RecordingData { return data; },
	        [batch_size, descriptor,
	         network_graph](network::PlasticityRule::TimedRecordingData const& data)
	            -> PlasticityRule::RecordingData {
		        PlasticityRule::TimedRecordingData logical_data;
		        for (auto const& logical_proj :
		             network_graph.get_network()->plasticity_rules.at(descriptor).projections) {
			        for (auto const& [name, d] : data.data_per_synapse) {
				        auto& local_logical_data = logical_data.data_per_synapse[name];
				        for (size_t index = 0; index < network_graph.get_network()
				                                           ->projections.at(logical_proj)
				                                           .connections.size();
				             ++index) {
					        auto const hardware_connections =
					            network_graph.get_projection_translation().equal_range(
					                std::make_pair(logical_proj, index));
					        for (auto const& [_, hardware_index] :
					             boost::make_iterator_range(hardware_connections)) {
						        auto const extractor = [&](auto const& dd) {
							        extract_plasticity_rule_recording_data_per_synapse(
							            local_logical_data,
							            network_graph.get_network()
							                ->plasticity_rules.at(descriptor)
							                .timer.num_periods,
							            logical_proj, network_graph, batch_size,
							            hardware_index.second, index, dd);
						        };
						        std::visit(extractor, d.at(hardware_index.first));
					        }
				        }
			        }
		        }
		        logical_data.data_array = data.data_array;
		        return logical_data;
	        }},
	    hardware_data);
}

} // namespace grenade::vx::logical_network
