#include "grenade/vx/network/population.h"

namespace grenade::vx::network {

Population::Population(
    Neurons const& neurons, bool const enable_record_spikes, bool const enable_record_v) :
    neurons(neurons), enable_record_spikes(enable_record_spikes), enable_record_v(enable_record_v)
{}

Population::Population(
    Neurons&& neurons, bool const enable_record_spikes, bool const enable_record_v) :
    neurons(std::move(neurons)),
    enable_record_spikes(enable_record_spikes),
    enable_record_v(enable_record_v)
{}


ExternalPopulation::ExternalPopulation(size_t const size) : size(size) {}

} // namespace grenade::vx::network
