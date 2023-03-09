#pragma once
#include "grenade/vx/common/time.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_atomic/network.h"
#include "grenade/vx/network/placed_atomic/network_graph.h"
#include "grenade/vx/signal_flow/io_data_map.h"
#include "halco/hicann-dls/vx/v3/neuron.h"

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include "hate/timer.h"
#include <log4cxx/logger.h>
#endif

namespace grenade::vx::network::placed_atomic GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_ATOMIC {

/**
 * Extract to be recorded observable data of a plasticity rule.
 * @param data Data containing observables
 * @param network_graph Network graph to use for vertex descriptor lookup of the observables
 * @param descriptor Descriptor to plasticity rule to extract observable data for
 * @return Observable data per batch entry
 */
PlasticityRule::RecordingData GENPYBIND(visible) extract_plasticity_rule_recording_data(
    signal_flow::IODataMap const& data,
    NetworkGraph const& network_graph,
    PlasticityRuleDescriptor descriptor) SYMBOL_VISIBLE;

} // namespace grenade::vx::network::placed_atomic
