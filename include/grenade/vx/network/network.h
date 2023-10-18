#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/background_source_population.h"
#include "grenade/vx/network/cadc_recording.h"
#include "grenade/vx/network/external_source_population.h"
#include "grenade/vx/network/madc_recording.h"
#include "grenade/vx/network/pad_recording.h"
#include "grenade/vx/network/plasticity_rule.h"
#include "grenade/vx/network/plasticity_rule_on_network.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/population_on_network.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/projection_on_network.h"
#include "hate/visibility.h"
#include <chrono>
#include <iosfwd>
#include <map>
#include <memory>
#include <variant>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include <pybind11/chrono.h>
#endif

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Placed but not routed network consisting of populations and projections.
 */
struct GENPYBIND(visible, holder_type("std::shared_ptr<grenade::vx::network::Network>")) Network
{
	std::map<
	    PopulationOnNetwork,
	    std::variant<Population, ExternalSourcePopulation, BackgroundSourcePopulation>> const
	    populations;
	std::map<ProjectionOnNetwork, Projection> const projections;
	std::optional<MADCRecording> const madc_recording;
	std::optional<CADCRecording> const cadc_recording;
	std::optional<PadRecording> const pad_recording;
	std::map<PlasticityRuleOnNetwork, PlasticityRule> const plasticity_rules;

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

} // namespace grenade::vx::network
