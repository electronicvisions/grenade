#pragma once

#include "grenade/vx/network/abstract/multicompartment_placement_algorithm.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Dummy Placement Algorithm, that returns the given Coordinate System.
 */
struct GENPYBIND(visible) SYMBOL_VISIBLE PlacementAlgorithmDummy : public PlacementAlgorithm
{
	PlacementAlgorithmDummy() = default;
	PlacementAlgorithmDummy(CoordinateSystem const& configuration);

	CoordinateSystem coordinate_system;

	AlgorithmResult run(
	    CoordinateSystem const& configuration_start,
	    Neuron const& neuron_in,
	    ResourceManager const& resources);
};

} // namespace abstract
} // namespace grenade::vx::network