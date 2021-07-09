#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/madc_recording.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "hate/visibility.h"
#include <map>
#include <memory>
#include <variant>

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace network {

/**
 * Placed but not routed network consisting of populations and projections.
 */
struct GENPYBIND(visible, holder_type("std::shared_ptr<grenade::vx::network::Network>")) Network
{
	std::map<
	    PopulationDescriptor,
	    std::variant<Population, ExternalPopulation, BackgroundSpikeSourcePopulation>> const
	    populations;
	std::map<ProjectionDescriptor, Projection> const projections;
	std::optional<MADCRecording> const madc_recording;

	bool operator==(Network const& other) const SYMBOL_VISIBLE;
	bool operator!=(Network const& other) const SYMBOL_VISIBLE;
};

} // namespace network

} // namespace grenade::vx
