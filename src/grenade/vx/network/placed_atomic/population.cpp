#include "grenade/vx/network/placed_atomic/population.h"

#include <ostream>

namespace grenade::vx::network::placed_atomic {

Population::Population(Neurons const& neurons, EnableRecordSpikes const& enable_record_spikes) :
    neurons(neurons), enable_record_spikes(enable_record_spikes)
{}

bool Population::operator==(Population const& other) const
{
	return neurons == other.neurons && enable_record_spikes == other.enable_record_spikes;
}

bool Population::operator!=(Population const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, Population const& population)
{
	if (population.neurons.size() != population.enable_record_spikes.size()) {
		throw std::runtime_error("Population not valid.");
	}
	os << "Population(\n";
	os << "\tsize: " << population.neurons.size() << "\n";
	for (size_t i = 0; i < population.neurons.size(); ++i) {
		os << "\t" << population.neurons.at(i) << ", enable_record_spikes: " << std::boolalpha
		   << population.enable_record_spikes.at(i) << "\n";
	}
	os << ")";
	return os;
}


ExternalPopulation::ExternalPopulation(size_t const size) : size(size) {}

bool ExternalPopulation::operator==(ExternalPopulation const& other) const
{
	return size == other.size;
}

bool ExternalPopulation::operator!=(ExternalPopulation const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, ExternalPopulation const& population)
{
	os << "ExternalPopulation(size: " << population.size << ")";
	return os;
}


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

} // namespace grenade::vx::network::placed_atomic
