#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/logical_network/network_graph.h"
#include "grenade/vx/network/network_graph.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "haldls/vx/v3/systime.h"
#include <map>
#include <tuple>
#include <vector>

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace logical_network GENPYBIND_MODULE {

/**
 * Extract spikes corresponding to neurons in the network.
 * Spikes which don't correspond to a neuron in the network are ignored.
 * @param data Data containing spike labels
 * @param network_graph Network graph to use for matching of spike labels to neurons
 * @return Time-series neuron spike data per batch entry
 */
std::vector<std::map<
    std::tuple<PopulationDescriptor, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
    std::vector<haldls::vx::v3::ChipTime>>>
extract_neuron_spikes(
    IODataMap const& data,
    NetworkGraph const& network_graph,
    network::NetworkGraph const& hardware_network_graph) SYMBOL_VISIBLE;

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
    IODataMap const& data,
    NetworkGraph const& network_graph,
    network::NetworkGraph const& hardware_network_graph,
    PlasticityRuleDescriptor descriptor) SYMBOL_VISIBLE;

} // namespace logical_network

} // namespace grenade::vx
