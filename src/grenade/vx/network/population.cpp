#include "grenade/vx/network/population.h"

namespace grenade::vx::network {

Population::Population(Neurons const& neurons, EnableRecordSpikes const& enable_record_spikes) :
    neurons(neurons), enable_record_spikes(enable_record_spikes)
{
	if (neurons.size() != enable_record_spikes.size()) {
		throw std::runtime_error("Spike recorder enable mask not same size as neuron count.");
	}
}


ExternalPopulation::ExternalPopulation(size_t const size) : size(size) {}

} // namespace grenade::vx::network
