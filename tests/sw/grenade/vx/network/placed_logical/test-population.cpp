#include <gtest/gtest.h>

#include "grenade/vx/network/placed_logical/population.h"

using namespace grenade::vx::network::placed_logical;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(network_Population, General)
{
	Population population({
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS(Enum(0))),
	        Population::Neuron::Compartments{
	            {CompartmentOnLogicalNeuron(),
	             Population::Neuron::Compartment{
	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}),
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS(Enum(1))),
	        Population::Neuron::Compartments{
	            {CompartmentOnLogicalNeuron(),
	             Population::Neuron::Compartment{
	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}),
	});

	Population population_copy(population);
	EXPECT_EQ(population, population_copy);
	population_copy.neurons.at(1)
	    .compartments.at(CompartmentOnLogicalNeuron())
	    .spike_master->enable_record_spikes = true;
	EXPECT_NE(population, population_copy);
	population_copy.neurons.at(1)
	    .compartments.at(CompartmentOnLogicalNeuron())
	    .spike_master->enable_record_spikes = false;
	EXPECT_EQ(population, population_copy);
	population_copy.neurons.at(1).coordinate = LogicalNeuronOnDLS(
	    LogicalNeuronCompartments(
	        {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	    AtomicNeuronOnDLS(Enum(2)));
	EXPECT_NE(population, population_copy);
}

TEST(network_ExternalPopulation, General)
{
	ExternalPopulation population(2);

	ExternalPopulation population_copy(population);
	EXPECT_EQ(population, population_copy);
	population_copy.size = 3;
	EXPECT_NE(population, population_copy);
}
