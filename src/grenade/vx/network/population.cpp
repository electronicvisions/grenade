#include "grenade/vx/network/population.h"

#include "hate/indent.h"
#include "hate/join.h"
#include <ostream>
#include <set>

namespace grenade::vx::network {

Population::Neuron::Compartment::SpikeMaster::SpikeMaster(
    size_t neuron_on_compartment, bool enable_record_spikes) :
    neuron_on_compartment(neuron_on_compartment), enable_record_spikes(enable_record_spikes)
{}

bool Population::Neuron::Compartment::SpikeMaster::operator==(SpikeMaster const& other) const
{
	return neuron_on_compartment == other.neuron_on_compartment &&
	       enable_record_spikes == other.enable_record_spikes;
}

bool Population::Neuron::Compartment::SpikeMaster::operator!=(SpikeMaster const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(
    std::ostream& os, Population::Neuron::Compartment::SpikeMaster const& config)
{
	std::stringstream ss;
	ss << "SpikeMaster(neuron_on_compartment: " << config.neuron_on_compartment << std::boolalpha
	   << ", enable_record_spikes: " << config.enable_record_spikes << ")";
	os << ss.str();
	return os;
}


Population::Neuron::Compartment::Compartment(
    std::optional<SpikeMaster> const& spike_master, Receptors const& receptors) :
    spike_master(spike_master), receptors(receptors)
{}

bool Population::Neuron::Compartment::operator==(Compartment const& other) const
{
	return spike_master == other.spike_master && receptors == other.receptors;
}

bool Population::Neuron::Compartment::operator!=(Compartment const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, Population::Neuron::Compartment const& config)
{
	os << "Compartment(\n";
	os << "\tspike_master: ";
	if (config.spike_master) {
		os << *(config.spike_master);
	} else {
		os << "None";
	}
	os << "\n";
	os << "\treceptors:\n";
	for (auto const& receptor : config.receptors) {
		os << "\t\t[" << hate::join_string(receptor.begin(), receptor.end(), ", ") << "]\n";
	}
	os << ")";
	return os;
}


Population::Neuron::Neuron(Coordinate const& coordinate, Compartments const& compartments) :
    coordinate(coordinate), compartments(compartments)
{}

bool Population::Neuron::operator==(Neuron const& other) const
{
	return coordinate == other.coordinate && compartments == other.compartments;
}

bool Population::Neuron::operator!=(Neuron const& other) const
{
	return !(*this == other);
}

bool Population::Neuron::valid() const
{
	for (auto const& [index, compartment] : compartments) {
		// unknown compartment index
		if (!coordinate.get_placed_compartments().contains(index)) {
			return false;
		}
		if (compartment.spike_master) {
			// neuron on compartment out of range
			if (compartment.spike_master->neuron_on_compartment >=
			    coordinate.get_placed_compartments().at(index).size()) {
				return false;
			}
		}
		// wrong number of receptor collections compared to neurons in compartment
		if (compartment.receptors.size() != coordinate.get_placed_compartments().at(index).size()) {
			return false;
		}
	}
	return true;
}

std::ostream& operator<<(std::ostream& os, Population::Neuron const& config)
{
	os << "Neuron(\n";
	os << "\tcoordinate: " << config.coordinate;
	os << "\tcompartments:\n";
	for (auto const& compartment : config.compartments) {
		std::stringstream ss;
		ss << compartment.first << ": " << compartment.second;
		os << hate::indent(ss.str(), "\t\t") << "\n";
	}
	os << ")";
	return os;
}


Population::Population(Neurons const& neurons) : neurons(neurons) {}

bool Population::operator==(Population const& other) const
{
	return neurons == other.neurons;
}

bool Population::operator!=(Population const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, Population const& population)
{
	if (!population.valid()) {
		throw std::runtime_error("Population not valid.");
	}
	os << "Population(\n";
	os << "\tsize: " << population.neurons.size() << "\n";
	std::stringstream ss;
	for (size_t i = 0; i < population.neurons.size(); ++i) {
		ss << population.neurons.at(i) << "\n";
	}
	os << hate::indent(ss.str(), "\t");
	os << ")";
	return os;
}

bool Population::valid() const
{
	// check that all neurons are valid
	if (!std::all_of(
	        neurons.begin(), neurons.end(), [](auto const& neuron) { return neuron.valid(); })) {
		return false;
	}
	// check that atomic neurons in logical neurons are unique
	return get_atomic_neurons().size() == get_atomic_neurons_size();
}

std::set<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> Population::get_atomic_neurons() const
{
	std::set<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> unique;
	for (auto const& neuron : neurons) {
		for (auto const& [_, compartment] : neuron.coordinate.get_placed_compartments()) {
			unique.insert(compartment.begin(), compartment.end());
		}
	}
	return unique;
}

size_t Population::get_atomic_neurons_size() const
{
	size_t size = 0;
	for (auto const& neuron : neurons) {
		for (auto const& [_, compartment] : neuron.coordinate.get_placed_compartments()) {
			size += compartment.size();
		}
	}
	return size;
}

} // namespace grenade::vx::network
