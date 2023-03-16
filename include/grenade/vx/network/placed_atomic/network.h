#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_atomic/cadc_recording.h"
#include "grenade/vx/network/placed_atomic/madc_recording.h"
#include "grenade/vx/network/placed_atomic/plasticity_rule.h"
#include "grenade/vx/network/placed_atomic/population.h"
#include "grenade/vx/network/placed_atomic/projection.h"
#include "hate/visibility.h"
#include <chrono>
#include <iosfwd>
#include <map>
#include <memory>
#include <variant>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include <pybind11/chrono.h>
#endif

namespace grenade::vx::network::placed_atomic GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_ATOMIC {

/**
 * Placed but not routed network consisting of populations and projections.
 */
struct GENPYBIND(
    visible, holder_type("std::shared_ptr<grenade::vx::network::placed_atomic::Network>")) Network
{
	std::map<
	    PopulationDescriptor,
	    std::variant<Population, ExternalPopulation, BackgroundSpikeSourcePopulation>> const
	    populations;
	std::map<ProjectionDescriptor, Projection> const projections;
	std::optional<MADCRecording> const madc_recording;
	std::optional<CADCRecording> const cadc_recording;
	std::map<PlasticityRuleDescriptor, PlasticityRule> const plasticity_rules;

	/**
	 * Duration spent during construction of network.
	 * This value is not compared in operator{==,!=}.
	 */
	std::chrono::microseconds const construction_duration;

	bool operator==(Network const& other) const SYMBOL_VISIBLE;
	bool operator!=(Network const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, Network const& network) SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network::placed_atomic
