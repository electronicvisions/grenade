#pragma once

#include <vector>

namespace grenade::vx::network::abstract {

/**
 *Holds limits of placements spots of a compartment.
 */
struct CoordinateLimit
{
	int lower;
	int upper;
};

/**
 * Holds multiple limits of placement spots of one compartment.
 * Distinguishes between spots in top and bottom row.
 */
struct CoordinateLimits
{
	std::vector<CoordinateLimit> top;
	std::vector<CoordinateLimit> bottom;
};

} // namespace grenade::vx::network::abstract