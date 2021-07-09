#include "grenade/vx/network/population.h"

namespace grenade::vx::network {

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


ExternalPopulation::ExternalPopulation(size_t const size) : size(size) {}

bool ExternalPopulation::operator==(ExternalPopulation const& other) const
{
	return size == other.size;
}

bool ExternalPopulation::operator!=(ExternalPopulation const& other) const
{
	return !(*this == other);
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

} // namespace grenade::vx::network
