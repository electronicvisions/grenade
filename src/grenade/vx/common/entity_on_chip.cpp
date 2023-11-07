#include "grenade/vx/common/entity_on_chip.h"

#include <ostream>

namespace grenade::vx::common {

EntityOnChip::EntityOnChip(ChipCoordinate const& chip_coordinate) : chip_coordinate(chip_coordinate)
{}

bool EntityOnChip::operator==(EntityOnChip const& other) const
{
	return chip_coordinate == other.chip_coordinate;
}

bool EntityOnChip::operator!=(EntityOnChip const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, EntityOnChip const& entity)
{
	return os << "EntityOnChip(" << entity.chip_coordinate << ")";
}

} // namespace grenade::vx::common
