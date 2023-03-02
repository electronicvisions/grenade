#pragma once
#include "grenade/vx/common/time.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_atomic/network_graph.h"
#include "grenade/vx/network/placed_logical/network_graph.h"
#include "grenade/vx/signal_flow/io_data_map.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include <map>
#include <tuple>
#include <vector>

namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

/**
 * Extract spikes corresponding to neurons in the network.
 * Spikes which don't correspond to a neuron in the network are ignored.
 * @param data Data containing spike labels
 * @param network_graph Network graph to use for matching of logical to hardware neurons
 * @param hardware_network_graph Network graph to use for matching of spike labels to neurons
 * @return Time-series neuron spike data per batch entry
 */
std::vector<std::map<
    std::tuple<PopulationDescriptor, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
    std::vector<common::Time>>>
extract_neuron_spikes(
    signal_flow::IODataMap const& data,
    NetworkGraph const& network_graph,
    network::placed_atomic::NetworkGraph const& hardware_network_graph) SYMBOL_VISIBLE;

/**
 * Extract MADC samples to be recorded for a network.
 * @param data Data containing MADC samples
 * @param network_graph Network graph to use for matching of logical to hardware neurons
 * @param hardware_network_graph Network graph to use for vertex descriptor lookup of the MADC
 * samples
 * @return Time-series MADC sample data per batch entry
 */
std::vector<std::vector<std::pair<common::Time, haldls::vx::v3::MADCSampleFromChip::Value>>>
extract_madc_samples(
    signal_flow::IODataMap const& data,
    NetworkGraph const& network_graph,
    network::placed_atomic::NetworkGraph const& hardware_network_graph) SYMBOL_VISIBLE;

/**
 * Extract CADC samples to be recorded for a network.
 * @param data Data containing CADC samples
 * @param network_graph Network graph to use for matching of logical to hardware neurons
 * @param hardware_network_graph Network graph to use for vertex descriptor lookup of the CADC
 * samples
 * @return Time-series CADC sample data per batch entry. Samples are sorted by their ChipTime per
 * batch-entry and contain their corresponding location alongside the ADC value.
 */
std::vector<std::vector<std::tuple<
    common::Time,
    PopulationDescriptor,
    size_t,
    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
    size_t,
    signal_flow::Int8>>>
extract_cadc_samples(
    signal_flow::IODataMap const& data,
    NetworkGraph const& network_graph,
    network::placed_atomic::NetworkGraph const& hardware_network_graph) SYMBOL_VISIBLE;

/**
 * Extract to be recorded observable data of a plasticity rule.
 * @param data Data containing observables
 * @param network_graph Network graph to use for hardware to logical network translation
 * @param hardware_network_graph Network graph to use for vertex descriptor lookup of the
 * observables
 * @param descriptor Descriptor to plasticity rule to extract observable data for
 * @return Observable data per batch entry
 */
PlasticityRule::RecordingData GENPYBIND(visible) extract_plasticity_rule_recording_data(
    signal_flow::IODataMap const& data,
    NetworkGraph const& network_graph,
    network::placed_atomic::NetworkGraph const& hardware_network_graph,
    PlasticityRuleDescriptor descriptor) SYMBOL_VISIBLE;

} // namespace grenade::vx::network::placed_logical
