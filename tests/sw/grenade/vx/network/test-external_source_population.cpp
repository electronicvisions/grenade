#include <gtest/gtest.h>

#include "grenade/vx/network/external_source_population.h"

using namespace grenade::vx::network;

TEST(network_ExternalSourcePopulation, General)
{
	ExternalSourcePopulation population(
	    {ExternalSourcePopulation::Neuron(false), ExternalSourcePopulation::Neuron(true)});

	ExternalSourcePopulation population_copy(population);
	EXPECT_EQ(population, population_copy);
	population_copy.neurons = {
	    ExternalSourcePopulation::Neuron(false),
	    ExternalSourcePopulation::Neuron(true),
	    ExternalSourcePopulation::Neuron(false),
	};
	EXPECT_NE(population, population_copy);

	population_copy.neurons = {
	    ExternalSourcePopulation::Neuron(true), ExternalSourcePopulation::Neuron(true)};
	EXPECT_NE(population, population_copy);
}
