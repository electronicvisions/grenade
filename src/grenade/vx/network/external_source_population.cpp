#include "grenade/vx/network/external_source_population.h"

#include <ostream>

namespace grenade::vx::network {

ExternalSourcePopulation::ExternalSourcePopulation(
    size_t const size, common::EntityOnChip::ChipCoordinate const chip_coordinate) :
    common::EntityOnChip(chip_coordinate), size(size)
{}

bool ExternalSourcePopulation::operator==(ExternalSourcePopulation const& other) const
{
	return size == other.size && static_cast<common::EntityOnChip const&>(*this) ==
	                                 static_cast<common::EntityOnChip const&>(other);
}

bool ExternalSourcePopulation::operator!=(ExternalSourcePopulation const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, ExternalSourcePopulation const& population)
{
	os << "ExternalSourcePopulation(" << static_cast<common::EntityOnChip const&>(population)
	   << ", size: " << population.size << ")";
	return os;
}

} // namespace grenade::vx::network
