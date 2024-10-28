#include "grenade/vx/network/abstract/multicompartment_placement_spot.h"

namespace grenade::vx::network::abstract {

std::pair<size_t, NumberTopBottom> PlacementSpot::get_free_block_space() const
{
	if (x == 256 || x_opposite == 256) {
		return std::make_pair(0, NumberTopBottom(0, 0, 0));
	}

	size_t x_range = 0;
	size_t x_block = 256;

	if (direction == 1) {
		x_block = std::max(x, x_opposite);
		if (y == 0) {
			x_range = free_space.number_top - (std::max(x, x_block) - std::min(x, x_block));
		} else if (y == 1) {
			x_range = free_space.number_bottom - (std::max(x, x_block) - std::min(x, x_block));
		}
	} else if (direction == -1) {
		x_block = std::min(x, x_opposite);
		if (y == 0) {
			x_range = free_space.number_top - (std::max(x, x_block) - std::min(x, x_block));
		} else if (y == 1) {
			x_range = free_space.number_bottom - (std::max(x, x_block) - std::min(x, x_block));
		}
	}

	return std::make_pair(x_block, NumberTopBottom(2 * x_range, x_range, x_range));
}

std::ostream& operator<<(std::ostream& os, PlacementSpot const& spot)
{
	return os << "PlacementSpot: "
	          << "\n[x= " << spot.x << ", y= " << spot.y << ", dir= " << spot.direction
	          << ", found_opposite= " << spot.found_opposite << ", x_opposite= " << spot.x_opposite
	          << "]\n"
	          << spot.free_space << "\n";
}

} // namespace grenade::vx::network::abstract
