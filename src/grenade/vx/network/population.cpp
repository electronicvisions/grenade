#include "grenade/vx/network/population.h"

namespace grenade::vx::network {

Population::Population(
    Neurons const& neurons, bool const enable_record_spikes, RecordSource const record_source) :
    neurons(neurons), enable_record_spikes(enable_record_spikes), record_source(record_source)
{}

Population::Population(
    Neurons&& neurons, bool const enable_record_spikes, RecordSource const record_source) :
    neurons(std::move(neurons)),
    enable_record_spikes(enable_record_spikes),
    record_source(record_source)
{}


ExternalPopulation::ExternalPopulation(size_t const size) : size(size) {}

} // namespace grenade::vx::network
