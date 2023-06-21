#include "grenade/vx/network/atomic_neuron_on_network.h"

namespace grenade::vx::network {

AtomicNeuronOnNetwork::AtomicNeuronOnNetwork(
    PopulationDescriptor population,
    size_t neuron_on_population,
    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron compartment_on_neuron,
    size_t atomic_neuron_on_compartment) :
    population(population),
    neuron_on_population(neuron_on_population),
    compartment_on_neuron(compartment_on_neuron),
    atomic_neuron_on_compartment(atomic_neuron_on_compartment)
{}

bool AtomicNeuronOnNetwork::operator==(AtomicNeuronOnNetwork const& other) const
{
	return population == other.population && neuron_on_population == other.neuron_on_population &&
	       compartment_on_neuron == other.compartment_on_neuron &&
	       atomic_neuron_on_compartment == other.atomic_neuron_on_compartment;
}

bool AtomicNeuronOnNetwork::operator!=(AtomicNeuronOnNetwork const& other) const
{
	return !(*this == other);
}

bool AtomicNeuronOnNetwork::operator<=(AtomicNeuronOnNetwork const& other) const
{
	return std::tie(
	           population, neuron_on_population, compartment_on_neuron,
	           atomic_neuron_on_compartment) <=
	       std::tie(
	           other.population, other.neuron_on_population, other.compartment_on_neuron,
	           other.atomic_neuron_on_compartment);
}

bool AtomicNeuronOnNetwork::operator>=(AtomicNeuronOnNetwork const& other) const
{
	return std::tie(
	           population, neuron_on_population, compartment_on_neuron,
	           atomic_neuron_on_compartment) >=
	       std::tie(
	           other.population, other.neuron_on_population, other.compartment_on_neuron,
	           other.atomic_neuron_on_compartment);
}

bool AtomicNeuronOnNetwork::operator<(AtomicNeuronOnNetwork const& other) const
{
	return !(*this >= other);
}

bool AtomicNeuronOnNetwork::operator>(AtomicNeuronOnNetwork const& other) const
{
	return !(*this <= other);
}

std::ostream& operator<<(std::ostream& os, AtomicNeuronOnNetwork const& value)
{
	os << "AtomicNeuronOnNetwork(" << value.population << ", " << value.neuron_on_population << ", "
	   << value.compartment_on_neuron << ", " << value.atomic_neuron_on_compartment << ")";
	return os;
}

} // namespace grenade::vx::network
