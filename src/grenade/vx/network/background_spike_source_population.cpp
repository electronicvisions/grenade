#include "grenade/vx/network/background_spike_source_population.h"

#include <ostream>

namespace grenade::vx::network {

BackgroundSpikeSourcePopulation::BackgroundSpikeSourcePopulation(
    size_t const size, Coordinate const& coordinate, Config const& config) :
    size(size), coordinate(coordinate), config(config)
{}

bool BackgroundSpikeSourcePopulation::Config::operator==(
    BackgroundSpikeSourcePopulation::Config const& other) const
{
	return period == other.period && rate == other.rate && seed == other.seed && enable_random &&
	       other.enable_random;
}

bool BackgroundSpikeSourcePopulation::Config::operator!=(
    BackgroundSpikeSourcePopulation::Config const& other) const
{
	return !(*this == other);
}

bool BackgroundSpikeSourcePopulation::operator==(BackgroundSpikeSourcePopulation const& other) const
{
	return size == other.size && coordinate == other.coordinate && config == other.config;
}

bool BackgroundSpikeSourcePopulation::operator!=(BackgroundSpikeSourcePopulation const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, BackgroundSpikeSourcePopulation::Config const& config)
{
	os << "Config(" << config.period << ", " << config.rate << ", " << config.seed
	   << ", enable_random: " << std::boolalpha << config.enable_random << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, BackgroundSpikeSourcePopulation const& population)
{
	os << "BackgroundSpikeSourcePopulation(\n";
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
