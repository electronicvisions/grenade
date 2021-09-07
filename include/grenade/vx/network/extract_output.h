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
    std::map<halco::hicann_dls::vx::v2::AtomicNeuronOnDLS, std::vector<haldls::vx::v2::ChipTime>>>
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
		    std::map<int, pybind11::array_t<double>> ret;
		    if (spikes.empty()) {
			    return ret;
		    }
		    assert(spikes.size() == 1);
		    for (auto const& [neuron, times] : spikes.at(0)) {
			    pybind11::array_t<double> pytimes({static_cast<pybind11::ssize_t>(times.size())});
			    for (size_t i = 0; i < times.size(); ++i) {
				    pytimes.mutable_at(i) = convert_ms(times.at(i));
			    }
			    ret[neuron.toEnum().value()] = pytimes;
		    }
		    return ret;
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
		pybind11::array_t<float> times({static_cast<pybind11::ssize_t>(madc_samples.size())});
		pybind11::array_t<int> values({static_cast<pybind11::ssize_t>(madc_samples.size())});
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
