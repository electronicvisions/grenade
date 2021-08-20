#include "grenade/vx/logical_network/population.h"

#include "hate/join.h"
#include <ostream>

namespace grenade::vx::logical_network {

Population::Population(
    Neurons const& neurons,
    Receptors const& receptors,
    EnableRecordSpikes const& enable_record_spikes) :
    neurons(neurons), receptors(receptors), enable_record_spikes(enable_record_spikes)
{}

bool Population::operator==(Population const& other) const
{
	return neurons == other.neurons && receptors == other.receptors &&
	       enable_record_spikes == other.enable_record_spikes;
}

bool Population::operator!=(Population const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, Population const& population)
{
	if ((population.neurons.size() != population.enable_record_spikes.size()) ||
	    (population.receptors.size() != population.enable_record_spikes.size())) {
		throw std::runtime_error("Population not valid.");
	}
	os << "Population(\n";
	os << "\tsize: " << population.neurons.size() << "\n";
	for (size_t i = 0; i < population.neurons.size(); ++i) {
		os << "\t" << population.neurons.at(i) << ", receptors: ["
		   << hate::join_string(
		          population.receptors.at(i).begin(), population.receptors.at(i).end(), ", ")
		   << "], enable_record_spikes: " << std::boolalpha << population.enable_record_spikes.at(i)
		   << "\n";
	}
	os << ")";
	return os;
}

} // namespace grenade::vx::logical_network
