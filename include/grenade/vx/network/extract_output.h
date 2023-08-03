#pragma once
#include "grenade/vx/common/time.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/atomic_neuron_on_network.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/plasticity_rule_on_network.h"
#include "grenade/vx/network/population_on_network.h"
#include "grenade/vx/network/projection_on_network.h"
#include "grenade/vx/signal_flow/io_data_map.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include <map>
#include <tuple>
#include <vector>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include "hate/timer.h"
#include <log4cxx/logger.h>
#endif

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Extract spikes corresponding to neurons in the network.
 * Spikes which don't correspond to a neuron in the network are ignored.
 * @param data Data containing spike labels
 * @param network_graph Network graph to use for matching of logical to hardware neurons
 * @return Time-series neuron spike data per batch entry
 */
std::vector<std::map<
    std::tuple<PopulationOnNetwork, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
    std::vector<common::Time>>>
extract_neuron_spikes(signal_flow::IODataMap const& data, NetworkGraph const& network_graph)
    SYMBOL_VISIBLE;

/**
 * Extract MADC samples to be recorded for a network.
 * @param data Data containing MADC samples
 * @param network_graph Network graph to use for matching of logical to hardware neurons
 * @return Time-series MADC sample data per batch entry. Samples are sorted by their ChipTime per
 * batch-entry and contain their corresponding location alongside the ADC value.
 */
std::vector<std::vector<
    std::tuple<common::Time, AtomicNeuronOnNetwork, haldls::vx::v3::MADCSampleFromChip::Value>>>
extract_madc_samples(signal_flow::IODataMap const& data, NetworkGraph const& network_graph)
    SYMBOL_VISIBLE;

/**
 * Extract CADC samples to be recorded for a network.
 * @param data Data containing CADC samples
 * @param network_graph Network graph to use for matching of logical to hardware neurons
 * @return Time-series CADC sample data per batch entry. Samples are sorted by their ChipTime per
 * batch-entry and contain their corresponding location alongside the ADC value.
 */
std::vector<std::vector<std::tuple<common::Time, AtomicNeuronOnNetwork, signal_flow::Int8>>>
extract_cadc_samples(signal_flow::IODataMap const& data, NetworkGraph const& network_graph)
    SYMBOL_VISIBLE;

/**
 * Extract to be recorded observable data of a plasticity rule.
 * @param data Data containing observables
 * @param network_graph Network graph to use for hardware to logical network translation
 * @param descriptor Descriptor to plasticity rule to extract observable data for
 * @return Observable data per batch entry
 */
PlasticityRule::RecordingData GENPYBIND(visible) extract_plasticity_rule_recording_data(
    signal_flow::IODataMap const& data,
    NetworkGraph const& network_graph,
    PlasticityRuleOnNetwork descriptor) SYMBOL_VISIBLE;


/**
 * PyNN's format expects times in floating-point ms, neuron (compartment)s as integer representation
 * of the enum values and sample values as integer values. Currently, only batch-size one is
 * supported, i.e. one time sequence.
 */
GENPYBIND_MANUAL({
	auto const convert_ms = [](auto const t) {
		return static_cast<float>(t) / 1000. /
		       static_cast<float>(grenade::vx::common::Time::fpga_clock_cycles_per_us);
	};
	auto const extract_neuron_spikes =
	    [convert_ms](
	        grenade::vx::signal_flow::IODataMap const& data,
	        grenade::vx::network::NetworkGraph const& network_graph) {
		    hate::Timer timer;
		    auto logger = log4cxx::Logger::getLogger("pygrenade.network.extract_neuron_spikes");
		    auto const spikes = grenade::vx::network::extract_neuron_spikes(data, network_graph);
		    std::vector<std::map<std::tuple<int, int, int>, pybind11::array_t<double>>> ret(
		        spikes.size());
		    for (size_t b = 0; b < spikes.size(); ++b) {
			    for (auto const& [neuron, times] : spikes.at(b)) {
				    pybind11::array_t<double> pytimes(static_cast<pybind11::ssize_t>(times.size()));
				    for (size_t i = 0; i < times.size(); ++i) {
					    pytimes.mutable_at(i) = convert_ms(times.at(i));
				    }
				    auto const& [descriptor, nrn_on_pop, comp_on_nrn] = neuron;
				    ret.at(b)[std::tuple<int, int, int>(descriptor, nrn_on_pop, comp_on_nrn)] =
				        pytimes;
			    }
		    }
		    LOG4CXX_TRACE(logger, "Execution duration: " << timer.print() << ".");
		    return ret;
	    };
	auto const extract_neuron_samples = [convert_ms](auto const& samples) {
		std::vector<std::tuple<
		    pybind11::array_t<float>, pybind11::array_t<int>, pybind11::array_t<int>,
		    pybind11::array_t<int>, pybind11::array_t<int>, pybind11::array_t<int>>>
		    ret(samples.size());
		for (size_t b = 0; b < samples.size(); ++b) {
			auto const madc_samples = samples.at(b);
			if (madc_samples.empty()) {
				ret.at(b) = std::make_tuple(
				    pybind11::array_t<float>(0), pybind11::array_t<int>(0),
				    pybind11::array_t<int>(0), pybind11::array_t<int>(0), pybind11::array_t<int>(0),
				    pybind11::array_t<int>(0));
				continue;
			}
			pybind11::array_t<float> times(static_cast<pybind11::ssize_t>(madc_samples.size()));
			pybind11::array_t<int> descriptor(static_cast<pybind11::ssize_t>(madc_samples.size()));
			pybind11::array_t<int> nrn_on_pop(static_cast<pybind11::ssize_t>(madc_samples.size()));
			pybind11::array_t<int> comp_on_nrn(static_cast<pybind11::ssize_t>(madc_samples.size()));
			pybind11::array_t<int> an_on_comp(static_cast<pybind11::ssize_t>(madc_samples.size()));
			pybind11::array_t<int> values(static_cast<pybind11::ssize_t>(madc_samples.size()));
			for (size_t i = 0; i < madc_samples.size(); ++i) {
				auto const& sample = madc_samples.at(i);
				times.mutable_at(i) = convert_ms(std::get<0>(sample));
				descriptor.mutable_at(i) = std::get<1>(sample).population.value();
				nrn_on_pop.mutable_at(i) = std::get<1>(sample).neuron_on_population;
				comp_on_nrn.mutable_at(i) = std::get<1>(sample).compartment_on_neuron.value();
				an_on_comp.mutable_at(i) = std::get<1>(sample).atomic_neuron_on_compartment;
				values.mutable_at(i) = std::get<2>(sample).value();
			}
			ret.at(b) =
			    std::make_tuple(times, descriptor, nrn_on_pop, comp_on_nrn, an_on_comp, values);
		}
		return ret;
	};
	auto const extract_madc_samples = [extract_neuron_samples](
	                                      grenade::vx::signal_flow::IODataMap const& data,
	                                      grenade::vx::network::NetworkGraph const& network_graph) {
		hate::Timer timer;
		auto logger = log4cxx::Logger::getLogger("pygrenade.network.extract_madc_samples");
		auto const ret =
		    extract_neuron_samples(grenade::vx::network::extract_madc_samples(data, network_graph));
		LOG4CXX_TRACE(logger, "Execution duration: " << timer.print() << ".");
		return ret;
	};
	auto const extract_cadc_samples = [extract_neuron_samples](
	                                      grenade::vx::signal_flow::IODataMap const& data,
	                                      grenade::vx::network::NetworkGraph const& network_graph) {
		hate::Timer timer;
		auto logger = log4cxx::Logger::getLogger("pygrenade.network.extract_cadc_samples");
		auto const ret =
		    extract_neuron_samples(grenade::vx::network::extract_cadc_samples(data, network_graph));
		LOG4CXX_TRACE(logger, "Execution duration: " << timer.print() << ".");
		return ret;
	};
	parent.def(
	    "extract_neuron_spikes", extract_neuron_spikes, pybind11::arg("data"),
	    pybind11::arg("network_graph"));
	parent.def(
	    "extract_madc_samples", extract_madc_samples, pybind11::arg("data"),
	    pybind11::arg("network_graph"));
	parent.def(
	    "extract_cadc_samples", extract_cadc_samples, pybind11::arg("data"),
	    pybind11::arg("network_graph"));
})

} // namespace grenade::vx::network
