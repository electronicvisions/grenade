#include "grenade/vx/network/background_source_population.h"

#include <ostream>

namespace grenade::vx::network {

BackgroundSourcePopulation::BackgroundSourcePopulation(
    size_t const size,
    Coordinate const& coordinate,
    Config const& config,
    common::EntityOnChip::ChipCoordinate const chip_coordinate) :
    common::EntityOnChip(chip_coordinate), size(size), coordinate(coordinate), config(config)
{}

bool BackgroundSourcePopulation::Config::operator==(
    BackgroundSourcePopulation::Config const& other) const
{
	return period == other.period && rate == other.rate && seed == other.seed && enable_random &&
	       other.enable_random;
}

bool BackgroundSourcePopulation::Config::operator!=(
    BackgroundSourcePopulation::Config const& other) const
{
	return !(*this == other);
}

bool BackgroundSourcePopulation::operator==(BackgroundSourcePopulation const& other) const
{
	return size == other.size && coordinate == other.coordinate && config == other.config &&
	       static_cast<common::EntityOnChip const&>(*this) ==
	           static_cast<common::EntityOnChip const&>(other);
}

bool BackgroundSourcePopulation::operator!=(BackgroundSourcePopulation const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, BackgroundSourcePopulation::Config const& config)
{
	os << "Config(" << config.period << ", " << config.rate << ", " << config.seed
	   << ", enable_random: " << std::boolalpha << config.enable_random << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, BackgroundSourcePopulation const& population)
{
	os << "BackgroundSourcePopulation(\n";
	os << "\t: " << static_cast<common::EntityOnChip const&>(population) << "\n";
	os << "\tsize: " << population.size << "\n";
	os << "\tcoordinate:\n";
	for (auto const& [hemisphere, padi_bus] : population.coordinate) {
		os << "\t\t"
		   << halco::hicann_dls::vx::v3::PADIBusOnDLS(padi_bus, hemisphere.toPADIBusBlockOnDLS())
		   << "\n";
	}
	os << "\t" << population.config << "\n)";
	return os;
}

} // namespace grenade::vx::network
