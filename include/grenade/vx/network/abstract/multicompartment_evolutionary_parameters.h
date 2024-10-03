#pragma once
#include <cstddef>
#include <iostream>

namespace grenade::vx::network::abstract {

struct EvolutionaryParameters
{
	// Seed for RNG
	size_t seed;

	// Time limit in seconds
	size_t time_limit;
	// Maximum number of runs (Total time = time_limit * run_limit)
	size_t run_limit;
	// Upper limit of coordinate system
	size_t x_max;

	// Number of runs which are executed in parallel.
	size_t parallel_threads = 1;

	// Population size
	size_t population_size;

	// Number of selected individuals for the hall of fame
	size_t number_hall_of_fame;
	// Number of contestants in each tournament
	size_t tournament_contestants;

	// Probability that two selected individuals mate
	double p_mate;
	// Size of the block of the genome that is exchanged
	size_t mating_block_size;

	// Probability that an individual is mutated
	double p_mutate_individual;
	// Probability for each bit of the indiviuduals genome to be flipped
	double p_mutate_gene;
	// Probability for the mutation at the start of each run to mutate an individual
	double p_mutate_initial;

	// Probability that an individual is mutatet
	double p_shift;
	// Minimum shift that can be performed
	size_t min_shift;
	// Maximum shift that can be performed
	size_t max_shift;

	// Probability to add columns to an individuals genome
	double p_add;
	// Probability to remove columns from an individuals genome
	double p_remove;
	// Whether the added/removed columns should be adjacent
	bool together_add_remove;
	// Number of added/removed columns
	size_t number_columns_add_remove;
	// Lower x-coordinate limit for adding/removing columns
	size_t lower_limit_add_remove;
	// Upper x-coordinate limit for adding/removing columns
	size_t upper_limit_add_remove;
};

std::ostream& operator<<(std::ostream& os, EvolutionaryParameters const& parameters);


} // namespace grenade::vx::network::abstract