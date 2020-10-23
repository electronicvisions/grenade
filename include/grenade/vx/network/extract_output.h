#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_graph.h"
#include "halco/hicann-dls/vx/v2/neuron.h"
#include "haldls/vx/v2/systime.h"

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace network {

/**
 * Extract spikes corresponding to neurons in the network.
 * Spikes which don't correspond to a neuron in the network are ignored.
 * @param data Data containing spike labels
 * @param network_graph Network graph to use for matching of spike labels to neurons
 * @return Time-series neuron spike data per batch entry
 */
std::vector<
    std::vector<std::pair<haldls::vx::v2::ChipTime, halco::hicann_dls::vx::v2::AtomicNeuronOnDLS>>>
extract_neuron_spikes(IODataMap const& data, NetworkGraph const& network_graph) SYMBOL_VISIBLE;


/**
 * Extract MADC samples to be recorded for a network.
 * @param data Data containing MADC samples
 * @param network_graph Network graph to use for vertex descriptor lookup of the MADC samples
 * @return Time-series MADC sample data per batch entry
 */
std::vector<
    std::vector<std::pair<haldls::vx::v2::ChipTime, haldls::vx::v2::MADCSampleFromChip::Value>>>
extract_madc_samples(IODataMap const& data, NetworkGraph const& network_graph) SYMBOL_VISIBLE;


/**
 * PyNN's format expects times in floating-point ms, neurons as integer representation of the
 * AtomicNeuron enum value and MADC sample values as integer values.
 * Currently, only batch-size one is supported, i.e. one time sequence.
 */
GENPYBIND_MANUAL({
	auto const convert_ms = [](auto const t) {
		return static_cast<float>(t) / 1000. /
		       static_cast<float>(grenade::vx::TimedSpike::Time::fpga_clock_cycles_per_us);
	};
	auto const extract_neuron_spikes =
	    [convert_ms](
	        grenade::vx::IODataMap const& data,
	        grenade::vx::network::NetworkGraph const& network_graph) {
		    auto const spikes = grenade::vx::network::extract_neuron_spikes(data, network_graph);
		    if (spikes.empty()) {
			    return std::tuple{pybind11::array_t<float>(0), pybind11::array_t<int>(0)};
		    }
		    assert(spikes.size() == 1);
		    auto const neuron_spikes = spikes.at(0);
		    pybind11::array_t<float> times({neuron_spikes.size()});
		    pybind11::array_t<int> neurons({neuron_spikes.size()});
		    for (size_t i = 0; i < neuron_spikes.size(); ++i) {
			    auto const& spike = neuron_spikes.at(i);
			    times.mutable_at(i) = convert_ms(spike.first);
			    neurons.mutable_at(i) = spike.second.toEnum().value();
		    }
		    return std::tuple{times, neurons};
	    };
	auto const extract_madc_samples = [convert_ms](
	                                      grenade::vx::IODataMap const& data,
	                                      grenade::vx::network::NetworkGraph const& network_graph) {
		auto const samples = grenade::vx::network::extract_madc_samples(data, network_graph);
		if (samples.empty()) {
			return std::tuple{pybind11::array_t<float>(0), pybind11::array_t<int>(0)};
		}
		assert(samples.size() == 1);
		auto const madc_samples = samples.at(0);
		pybind11::array_t<float> times({madc_samples.size()});
		pybind11::array_t<int> values({madc_samples.size()});
		for (size_t i = 0; i < madc_samples.size(); ++i) {
			auto const& sample = madc_samples.at(i);
			times.mutable_at(i) = convert_ms(sample.first);
			values.mutable_at(i) = sample.second.toEnum().value();
		}
		return std::tuple{times, values};
	};
	parent.def(
	    "extract_neuron_spikes", extract_neuron_spikes, pybind11::arg("data"),
	    pybind11::arg("network_graph"));
	parent.def(
	    "extract_madc_samples", extract_madc_samples, pybind11::arg("data"),
	    pybind11::arg("network_graph"));
})

} // namespace network

} // namespace grenade::vx
