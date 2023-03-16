#include <gtest/gtest.h>

#include "grenade/vx/network/placed_atomic/population.h"

using namespace grenade::vx::network::placed_atomic;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(network_Population, General)
{
	Population population({AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1))}, {true, false});

	Population population_copy(population);
	EXPECT_EQ(population, population_copy);
	population_copy.enable_record_spikes.at(1) = true;
	EXPECT_NE(population, population_copy);
	population_copy.enable_record_spikes.at(1) = false;
	EXPECT_EQ(population, population_copy);
	population_copy.neurons.at(1) = AtomicNeuronOnDLS(Enum(2));
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
