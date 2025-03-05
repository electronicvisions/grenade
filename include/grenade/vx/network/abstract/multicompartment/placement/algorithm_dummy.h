#pragma once

#include "grenade/vx/network/abstract/multicompartment/placement/algorithm.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Dummy Placement Algorithm, that returns the given Coordinate System.
 */
struct GENPYBIND(visible) SYMBOL_VISIBLE PlacementAlgorithmDummy : public PlacementAlgorithm
{
	PlacementAlgorithmDummy() = default;
	PlacementAlgorithmDummy(CoordinateSystem const& configuration);

	/**
	 * Clone the algorithm. Only clones the initial state of the algorithm. New algorithm is in
	 * state as after reset(). Since PlacementAlgorithm is the abstract base class the algorithm is
	 * passed as a pointer. This allows to pass the algorithm polymorphically to functions.
	 * @return Unique pointer to copy of the algorithm.
	 */
	std::unique_ptr<PlacementAlgorithm> clone() const;

	/**
	 * Reset all members of the algorithm. Used during testing.
	 */
	void reset();

	CoordinateSystem coordinate_system;

	AlgorithmResult run(
	    CoordinateSystem const& configuration_start,
	    Neuron const& neuron_in,
	    ResourceManager const& resources);
};

} // namespace abstract
} // namespace grenade::vx::network