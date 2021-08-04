#include "grenade/vx/network/population.h"

namespace grenade::vx::network {

Population::Population(Neurons const& neurons, EnableRecordSpikes const& enable_record_spikes) :
    neurons(neurons), enable_record_spikes(enable_record_spikes)
{
	if (neurons.size() != enable_record_spikes.size()) {
		throw std::runtime_error("Spike recorder enable mask not same size as neuron count.");
	}
}

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

} // namespace grenade::vx::network
