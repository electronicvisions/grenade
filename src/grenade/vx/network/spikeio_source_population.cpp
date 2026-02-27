#include "grenade/vx/network/spikeio_source_population.h"
#include <ostream>


namespace grenade::vx::network {

SpikeIOSourcePopulation::Neuron::Neuron(
    halco::hicann_dls::vx::SpikeIOAddress label, bool enable_record_spikes) :
    label(label), enable_record_spikes(enable_record_spikes)
{
}


std::ostream& operator<<(std::ostream& os, SpikeIOSourcePopulation::Neuron const& value)
{
	os << "Neuron(" << (value.enable_record_spikes ? "rec=true" : "rec=false")
	   << ", label=" << value.label << ")";
	return os;
}


std::ostream& operator<<(std::ostream& os, SpikeIOSourcePopulation::Config const& value)
{
	os << ", loopback=" << (value.enable_internal_loopback ? "true" : "false")
	   << ", scaler=" << value.data_rate_scaler;
	return os;
}

SpikeIOSourcePopulation::SpikeIOSourcePopulation(
    std::vector<Neuron> neurons_, Config const& config_) :
    neurons(std::move(neurons_)), config(config_)
{
}


std::ostream& operator<<(std::ostream& os, SpikeIOSourcePopulation const& value)
{
	os << "SpikeIOSourcePopulation(size=" << value.neurons.size() << ", " << value.config << ")";
	return os;
}

} // namespace grenade::vx::network