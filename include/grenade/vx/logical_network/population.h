#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/population.h"
#include "halco/common/geometry.h"

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace logical_network GENPYBIND_MODULE {

typedef grenade::vx::network::Population Population;
typedef grenade::vx::network::ExternalPopulation ExternalPopulation;
typedef grenade::vx::network::BackgroundSpikeSourcePopulation BackgroundSpikeSourcePopulation;

/** Descriptor to be used to identify a population. */
struct GENPYBIND(inline_base("*")) PopulationDescriptor
    : public halco::common::detail::BaseType<PopulationDescriptor, size_t>
{
	constexpr explicit PopulationDescriptor(value_type const value = 0) : base_t(value) {}
};

} // namespace logical_network

GENPYBIND_MANUAL({
	parent.attr("logical_network").attr("Population") = parent.attr("Population");
	parent.attr("logical_network").attr("ExternalPopulation") = parent.attr("ExternalPopulation");
	parent.attr("logical_network").attr("BackgroundSpikeSourcePopulation") =
	    parent.attr("BackgroundSpikeSourcePopulation");
})

} // namespace grenade::vx

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::logical_network::PopulationDescriptor)

} // namespace std
