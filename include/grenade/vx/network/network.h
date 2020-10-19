#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
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
	std::map<PopulationDescriptor, std::variant<Population, ExternalPopulation>> const populations;
	std::map<ProjectionDescriptor, Projection> const projections;
};

} // namespace network

} // namespace grenade::vx
