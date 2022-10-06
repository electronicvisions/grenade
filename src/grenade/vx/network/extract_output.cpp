#include "grenade/vx/network/extract_output.h"

#include "halco/hicann-dls/vx/v3/event.h"

namespace grenade::vx::network {

std::vector<
    std::map<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, std::vector<haldls::vx::v3::ChipTime>>>
extract_neuron_spikes(IODataMap const& data, NetworkGraph const& network_graph)
{
	if (!network_graph.get_event_output_vertex()) {
		return std::vector<std::map<
		    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, std::vector<haldls::vx::v3::ChipTime>>>(
		    data.batch_size());
	}
	// generate reverse lookup table from spike label to neuron coordinate
	std::map<halco::hicann_dls::vx::v3::SpikeLabel, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>
	    label_lookup;
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
				assert(neurons.at(i).at(0));
				label_lookup[*(neurons.at(i).at(0))] = population.neurons.at(i);
			}
		}
	}
	// convert spikes
	auto const& spikes = std::get<std::vector<TimedSpikeFromChipSequence>>(
	    data.data.at(*network_graph.get_event_output_vertex()));
	std::vector<std::map<
	    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, std::vector<haldls::vx::v3::ChipTime>>>
	    ret(data.batch_size());
	assert(!spikes.size() || spikes.size() == data.batch_size());
	for (size_t b = 0; b < spikes.size(); ++b) {
		auto& local_ret = ret.at(b);
		for (auto const& spike : spikes.at(b)) {
			auto const label = spike.label;
			if (label_lookup.contains(label)) {
				local_ret[label_lookup.at(label)].push_back(spike.chip_time);
			}
		}
	}
	return ret;
}

std::vector<
    std::vector<std::pair<haldls::vx::v3::ChipTime, haldls::vx::v3::MADCSampleFromChip::Value>>>
extract_madc_samples(IODataMap const& data, NetworkGraph const& network_graph)
{
	if (!network_graph.get_madc_sample_output_vertex()) {
		std::vector<std::vector<
		    std::pair<haldls::vx::v3::ChipTime, haldls::vx::v3::MADCSampleFromChip::Value>>>
		    ret(data.batch_size());
		return ret;
	}
	// convert samples
	auto const& samples = std::get<std::vector<TimedMADCSampleFromChipSequence>>(
	    data.data.at(*network_graph.get_madc_sample_output_vertex()));
	std::vector<
	    std::vector<std::pair<haldls::vx::v3::ChipTime, haldls::vx::v3::MADCSampleFromChip::Value>>>
	    ret(data.batch_size());
	assert(!samples.size() || samples.size() == data.batch_size());
	for (size_t b = 0; b < samples.size(); ++b) {
		auto& local_ret = ret.at(b);
		for (auto const& sample : samples.at(b)) {
			local_ret.push_back({sample.chip_time, sample.value});
		}
	}
	return ret;
}

std::vector<std::vector<
    std::tuple<haldls::vx::v3::ChipTime, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, Int8>>>
extract_cadc_samples(IODataMap const& data, NetworkGraph const& network_graph)
{
	// convert samples
	std::vector<std::vector<
	    std::tuple<haldls::vx::v3::ChipTime, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, Int8>>>
	    ret(data.batch_size());
	for (auto const cadc_output_vertex : network_graph.get_cadc_sample_output_vertex()) {
		auto const& samples = std::get<std::vector<TimedDataSequence<std::vector<Int8>>>>(
		    data.data.at(cadc_output_vertex));
		assert(!samples.size() || samples.size() == data.batch_size());
		assert(boost::in_degree(cadc_output_vertex, network_graph.get_graph().get_graph()) == 1);
		auto const in_edges =
		    boost::in_edges(cadc_output_vertex, network_graph.get_graph().get_graph());
		auto const cadc_vertex =
		    boost::source(*in_edges.first, network_graph.get_graph().get_graph());
		auto const& vertex = std::get<vertex::CADCMembraneReadoutView>(
		    network_graph.get_graph().get_vertex_property(cadc_vertex));
		auto const& columns = vertex.get_columns();
		auto const& row = vertex.get_synram().toNeuronRowOnDLS();
		for (size_t b = 0; b < samples.size(); ++b) {
			auto& local_ret = ret.at(b);
			for (auto const& sample : samples.at(b)) {
				for (size_t j = 0; auto const& cs : columns) {
					for (auto const& column : cs) {
						local_ret.push_back(
						    {sample.chip_time,
						     halco::hicann_dls::vx::v3::AtomicNeuronOnDLS(
						         column.toNeuronColumnOnDLS(), row),
						     sample.data.at(j)});
						j++;
					}
				}
			}
		}
	}
	for (auto& batch : ret) {
		std::sort(batch.begin(), batch.end());
	}
	return ret;
}

} // namespace grenade::vx::network
