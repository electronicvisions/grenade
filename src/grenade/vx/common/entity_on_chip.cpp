#include "grenade/vx/common/entity_on_chip.h"

#include <ostream>

namespace grenade::vx::common {

EntityOnChip::EntityOnChip(ChipOnExecutor const& chip_on_executor) :
    chip_on_executor(chip_on_executor)
{}

bool EntityOnChip::operator==(EntityOnChip const& other) const
{
	return chip_on_executor == other.chip_on_executor;
}

bool EntityOnChip::operator!=(EntityOnChip const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, EntityOnChip const& entity)
{
	return os << "EntityOnChip(" << entity.chip_on_executor.first << ", "
	          << entity.chip_on_executor.second << ")";
}

} // namespace grenade::vx::common
