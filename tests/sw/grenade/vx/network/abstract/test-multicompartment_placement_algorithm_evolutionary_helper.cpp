#include "grenade/vx/network/abstract/multicompartment_evolutionary_helper.h"
#include <gtest/gtest.h>

using namespace grenade::vx::network::abstract;


TEST(EvolutionaryHelpers, SelectionNBest)
{
	std::mt19937 gen(0);

	typedef EoChrom<bool, double> Chrom;
	typedef EoPopulation<Chrom> Population;

	Population pop_a, pop_b;
	size_t pop_size = 10;
	size_t chrom_size = 90;

	for (size_t i = 0; i < pop_size; i++) {
		Chrom chrom(chrom_size);
		chrom.fitness(0);
		pop_a.push_back(chrom);
		pop_b.push_back(chrom);
	}

	EXPECT_EQ(pop_a, pop_b);

	size_t number_best = 5;
	SelectionNBest<Chrom> select_best(number_best, false);

	pop_b.clear();
	select_best(pop_a, pop_b);
	EXPECT_EQ(pop_a.size(), pop_size);
	EXPECT_EQ(pop_b.size(), number_best);

	// Check that each of the selected individuals has a higher fitness than unselected ones.
	double max_fitness_a = 0;
	for (auto individual : pop_a) {
		if (individual.fitness() > max_fitness_a) {
			max_fitness_a = individual.fitness();
		}
	}
	for (auto individual : pop_b) {
		EXPECT_GE(individual.fitness(), max_fitness_a);
	}

	EXPECT_THROW(select_best(pop_a, pop_a), std::logic_error);
}


TEST(EvolutionaryHelpers, SelectNRandom)
{
	std::mt19937 gen(0);

	typedef EoChrom<bool, double> Chrom;
	typedef EoPopulation<Chrom> Population;

	Population pop_a, pop_b;
	size_t pop_size = 10;
	size_t chrom_size = 90;

	for (size_t i = 0; i < pop_size; i++) {
		Chrom chrom(chrom_size);
		chrom.fitness(0);
		pop_a.push_back(chrom);
		pop_b.push_back(chrom);
	}

	SelectionNRandom<Chrom> select_two(2, true, gen);

	select_two(pop_a, pop_b);

	EXPECT_EQ(pop_b.size(), 2);
	EXPECT_EQ(pop_a.size(), pop_size - 2);
	EXPECT_EQ(pop_a[0].size(), chrom_size);

	pop_a.resize(1);
	EXPECT_THROW(select_two(pop_a, pop_b), std::range_error);

	pop_a.resize(pop_size);
	pop_b.clear();
	select_two(pop_a, pop_b);
	EXPECT_EQ(pop_b.size(), 2);
}


TEST(EvolutionaryHelpers, Tournament)
{
	std::mt19937 gen(0);

	typedef EoChrom<bool, double> Chrom;
	typedef EoPopulation<Chrom> Population;

	Population pop_a, pop_b;
	size_t pop_size = 10;
	size_t chrom_size = 90;

	for (size_t i = 0; i < pop_size; i++) {
		Chrom chrom(chrom_size);
		chrom.fitness(0);
		pop_a.push_back(chrom);
		pop_b.push_back(chrom);
	}

	size_t number_contestants = 2;
	size_t number_tournaments = pop_size - 5; // Arbitrary number
	Tournament<Chrom> tournament(number_contestants, number_tournaments, gen);

	tournament(pop_a, pop_b);
	EXPECT_EQ(pop_a.size(), pop_size);
	EXPECT_EQ(pop_b.size(), number_tournaments);
	EXPECT_NE(pop_a, pop_b);

	EXPECT_THROW(tournament(pop_a, pop_a), std::logic_error);
}


TEST(EvolutionaryHelpers, Mating)
{
	std::mt19937 gen(0);

	typedef EoChrom<bool, double> Chrom;
	typedef EoPopulation<Chrom> Population;

	Population pop_a, pop_b;
	size_t pop_size = 10;
	size_t chrom_size = 90;

	for (size_t i = 0; i < pop_size; i++) {
		Chrom chrom(chrom_size);
		chrom.fitness(0);
		pop_a.push_back(chrom);
		pop_b.push_back(chrom);
	}

	size_t mating_block_size = 10;
	double p_mate = 0.1;
	MatingExchangeBlocks<Chrom> mating(chrom_size, mating_block_size, p_mate, gen);

	pop_a.clear();
	size_t initial_size = pop_b.size();
	mating(pop_b, pop_a);
	std::swap(pop_b, pop_a);
	EXPECT_EQ(pop_b.size(), initial_size);
	EXPECT_NE(pop_a, pop_b);

	EXPECT_THROW(mating(pop_b, pop_b), std::logic_error);
}


TEST(EvolutionaryHelpers, MutationBitFlip)
{
	std::mt19937 gen(0);

	typedef EoChrom<bool, double> Chrom;
	typedef EoPopulation<Chrom> Population;

	Population pop_a;
	size_t pop_size = 10;
	size_t chrom_size = 90;

	for (size_t i = 0; i < pop_size; i++) {
		Chrom chrom(chrom_size);
		chrom.fitness(0);
		pop_a.push_back(chrom);
	}

	double p_mutate_individual = 0.1;
	double p_mutate_gene = 0.01;
	MutationBitFlip<Chrom> mutator(p_mutate_individual, p_mutate_gene, gen);

	size_t initial_size = pop_a.size();
	mutator(pop_a);
	EXPECT_EQ(pop_a.size(), initial_size);
	EXPECT_EQ(pop_a[0].size(), chrom_size);
}


TEST(EvolutionaryHelpers, MutationShift)
{
	std::mt19937 gen(0);

	typedef EoChrom<bool, double> Chrom;
	typedef EoPopulation<Chrom> Population;

	Population pop_a;
	size_t pop_size = 10;
	size_t chrom_size = 90;

	for (size_t i = 0; i < pop_size; i++) {
		Chrom chrom(chrom_size);
		chrom.fitness(0);
		pop_a.push_back(chrom);
	}

	double p_shift = 0.1;
	size_t min_shift = 1;
	size_t max_shift = 4;

	MutationShiftRound<Chrom> shift(p_shift, min_shift, max_shift, gen);

	size_t initial_size = pop_a.size();
	shift(pop_a);
	EXPECT_EQ(pop_a.size(), initial_size);
	EXPECT_EQ(pop_a[0].size(), chrom_size);
}


TEST(EvolutionaryHelpers, MutationAddColumns)
{
	std::mt19937 gen(0);

	typedef EoChrom<bool, double> Chrom;
	typedef EoPopulation<Chrom> Population;

	Population pop_a;
	size_t pop_size = 10;
	size_t chrom_size = 90;

	for (size_t i = 0; i < pop_size; i++) {
		Chrom chrom(chrom_size);
		chrom.fitness(0);
		pop_a.push_back(chrom);
	}

	double p_add = 0.1;
	size_t number_columns_add = 1;
	bool together = true;
	size_t lower_limit = 0;
	size_t upper_limit = 10;

	MutationAddColumns<Chrom> add_columns(
	    p_add, number_columns_add, together, lower_limit, upper_limit, gen);

	size_t initial_size = pop_a.size();
	add_columns(pop_a);
	EXPECT_EQ(pop_a.size(), initial_size);
	EXPECT_EQ(pop_a[0].size(), chrom_size);

	// Adding columns
	Chrom test_chrom(chrom_size);
	test_chrom.fitness(0);
	for (size_t i = 0; i < test_chrom.size(); i++) {
		test_chrom[i] = 1;
	}
	std::vector<size_t> positions = {2};
	add_columns.add_columns(test_chrom, positions);
	for (size_t i = 9 * positions.at(0); i < 9 * positions.at(0) + 9; i++) {
		EXPECT_EQ(test_chrom[i], 0);
	}
}


TEST(EvolutionaryHelpers, MutationRemoveColumns)
{
	std::mt19937 gen(0);

	typedef EoChrom<bool, double> Chrom;
	typedef EoPopulation<Chrom> Population;

	Population pop_a;
	size_t pop_size = 10;
	size_t chrom_size = 90;

	for (size_t i = 0; i < pop_size; i++) {
		Chrom chrom(chrom_size);
		chrom.fitness(0);
		pop_a.push_back(chrom);
	}

	double p_remove = 0.1;
	size_t number_columns_remove = 1;
	bool together = true;
	size_t lower_limit = 0;
	size_t upper_limit = 10;

	MutationRemoveColumns<Chrom> remove_columns(
	    p_remove, number_columns_remove, together, lower_limit, upper_limit, gen);

	size_t initial_size = pop_a.size();
	remove_columns(pop_a);
	EXPECT_EQ(pop_a.size(), initial_size);
	EXPECT_EQ(pop_a[0].size(), chrom_size);

	// Removing columns
	Chrom test_chrom(chrom_size);
	test_chrom.fitness(0);
	for (size_t i = 0; i < test_chrom.size(); i++) {
		test_chrom[i] = 1;
	}
	std::vector<size_t> positions = {2};
	remove_columns.remove_columns(test_chrom, positions);
	for (size_t i = test_chrom.size(); i > test_chrom.size() - 9 * number_columns_remove; i--) {
		EXPECT_EQ(test_chrom[i], 0);
	}
}