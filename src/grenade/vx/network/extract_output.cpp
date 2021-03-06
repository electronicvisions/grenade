#include "grenade/vx/network/extract_output.h"

namespace grenade::vx::network {

std::vector<
    std::vector<std::pair<haldls::vx::v2::ChipTime, halco::hicann_dls::vx::v2::AtomicNeuronOnDLS>>>
extract_neuron_spikes(IODataMap const& data, NetworkGraph const& network_graph)
{
	if (!network_graph.get_event_output_vertex()) {
		std::vector<std::vector<
		    std::pair<haldls::vx::v2::ChipTime, halco::hicann_dls::vx::v2::AtomicNeuronOnDLS>>>
		    ret(data.batch_size());
		return ret;
	}
	// generate reverse lookup table from spike label to neuron coordinate
	std::map<haldls::vx::v2::SpikeLabel, halco::hicann_dls::vx::v2::AtomicNeuronOnDLS> label_lookup;
	assert(network_graph.get_network());
	for (auto const& [descriptor, neurons] : network_graph.get_spike_labels()) {
		if (!std::holds_alternative<Population>(
		        network_graph.get_network()->populations.at(descriptor))) {
			continue;
		}
		auto const& population =
		    std::get<Population>(network_graph.get_network()->populations.at(descriptor));
		for (size_t i = 0; i < neurons.size(); ++i) {
			if (population.enable_record_spikes.at(i)) {
				// internal neurons only have one label assigned
				assert(neurons.at(i).size() == 1);
				label_lookup[neurons.at(i).at(0)] = population.neurons.at(i);
			}
		}
	}
	// convert spikes
	auto const& spikes = data.spike_event_output.at(*network_graph.get_event_output_vertex());
	std::vector<std::vector<
	    std::pair<haldls::vx::v2::ChipTime, halco::hicann_dls::vx::v2::AtomicNeuronOnDLS>>>
	    ret(spikes.size());
	for (size_t b = 0; b < spikes.size(); ++b) {
		auto& local_ret = ret.at(b);
		for (auto const& spike : spikes.at(b)) {
			auto const label = spike.get_label();
			if (label_lookup.contains(label)) {
				local_ret.push_back({spike.get_chip_time(), label_lookup.at(label)});
			}
		}
	}
	return ret;
}

std::vector<
    std::vector<std::pair<haldls::vx::v2::ChipTime, haldls::vx::v2::MADCSampleFromChip::Value>>>
extract_madc_samples(IODataMap const& data, NetworkGraph const& network_graph)
{
	if (!network_graph.get_madc_sample_output_vertex()) {
		std::vector<std::vector<
		    std::pair<haldls::vx::v2::ChipTime, haldls::vx::v2::MADCSampleFromChip::Value>>>
		    ret(data.batch_size());
		return ret;
	}
	// convert samples
	auto const& samples = data.madc_samples.at(*network_graph.get_madc_sample_output_vertex());
	std::vector<
	    std::vector<std::pair<haldls::vx::v2::ChipTime, haldls::vx::v2::MADCSampleFromChip::Value>>>
	    ret(samples.size());
	for (size_t b = 0; b < samples.size(); ++b) {
		auto& local_ret = ret.at(b);
		for (auto const& sample : samples.at(b)) {
			local_ret.push_back({sample.get_chip_time(), sample.get_value()});
		}
	}
	return ret;
}

} // namespace grenade::vx::network
