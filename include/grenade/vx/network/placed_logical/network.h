#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_logical/cadc_recording.h"
#include "grenade/vx/network/placed_logical/madc_recording.h"
#include "grenade/vx/network/placed_logical/plasticity_rule.h"
#include "grenade/vx/network/placed_logical/population.h"
#include "grenade/vx/network/placed_logical/projection.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <map>
#include <memory>
#include <variant>

namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

/**
 * Placed but not routed network consisting of populations and projections.
 */
struct GENPYBIND(
    visible, holder_type("std::shared_ptr<grenade::vx::network::placed_logical::Network>")) Network
{
	std::map<
	    PopulationDescriptor,
	    std::variant<Population, ExternalPopulation, BackgroundSpikeSourcePopulation>> const
	    populations;
	std::map<ProjectionDescriptor, Projection> const projections;
	std::optional<MADCRecording> const madc_recording;
	std::optional<CADCRecording> const cadc_recording;
	std::map<PlasticityRuleDescriptor, PlasticityRule> const plasticity_rules;

	bool operator==(Network const& other) const SYMBOL_VISIBLE;
	bool operator!=(Network const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, Network const& network) SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network::placed_logical
