#include <gtest/gtest.h>

#include "grenade/vx/network/external_source_population.h"

using namespace grenade::vx::network;

TEST(network_ExternalSourcePopulation, General)
{
	ExternalSourcePopulation population(2);

	ExternalSourcePopulation population_copy(population);
	EXPECT_EQ(population, population_copy);
	population_copy.size = 3;
	EXPECT_NE(population, population_copy);
}
