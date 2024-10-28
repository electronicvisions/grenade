#pragma once

#include "grenade/vx/network/abstract/multicompartment_top_bottom.h"
#include <iostream>

namespace grenade::vx::network::abstract {

/**
 * Helper class for the ruleset algorithm.
 * The PlacementSpot describes a part of the CoorinateSystem that is suitable for the placement of a
 * specific compartment.
 */
struct PlacementSpot
{
	// Column of the spot.
	size_t x = 256;
	// Row of the spot.
	size_t y = 2;

	// Direction from the starting point of in which the spot extends. Either -1 or 1.
	int direction = 0;

	// Wether the spot has neuron circuts in both rows.
	bool found_opposite = false;
	// Starting point of the neuron circuits in opposite row.
	size_t x_opposite = 256;

	// How far away in x-direction the spot is from a given reference point.
	size_t distance = 256;

	// Number of free circuits in the spot (i.e. size of the spot).
	NumberTopBottom free_space = NumberTopBottom(0, 0, 0);

	// Return the size of the block (adjacent circuits in both rows) in the spot.
	std::pair<size_t, NumberTopBottom> get_free_block_space() const;
};

std::ostream& operator<<(std::ostream& os, PlacementSpot const& spot) GENPYBIND(hidden);

} // namespace grenade::vs::network::abstract