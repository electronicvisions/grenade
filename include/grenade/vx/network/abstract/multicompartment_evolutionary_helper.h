#pragma once

#include "grenade/vx/network/abstract/evolutionary/multicompartment_evolutionary_chromosome.h"
#include "grenade/vx/network/abstract/evolutionary/multicompartment_evolutionary_population.h"
#include <algorithm>
#include <random>
#include <set>
#include <sstream>


namespace grenade::vx::network::abstract {

/**
 * Selects the N best individulas ordered by their fitness.
 * @tparam EOT Type of the chromosomes of the population.
 */
template <class EOT>
struct SelectionNBest
{
	/**
	 * Constructor.
	 * @param number Number of best elements to select.
	 * @param delete_selected Whether selected elements should be deleted from the source
	 population.

	 */
	SelectionNBest(size_t number, bool delete_selected);

	/**
	 * Selects the N best individuals ordered by their fitness.
	 * @param pop_source Populations from which individuals are selected.
	 * @param pop_target Population where selected individuals are returned. Population is cleared
	 * before adding individuals.
	 */
	void operator()(EoPopulation<EOT>& pop_source, EoPopulation<EOT>& pop_target);

private:
	size_t m_number_selection;
	bool m_delete_selected;
};


/**
 * Selects N random individuals out of a population of individuals.
 * @tparam EOT Type of the chromosomes of the population
 */
template <class EOT>
struct SelectionNRandom
{
	/**
	 * Constructor.
	 * @param selection Number of elements to select.
	 * @param delete_selected Whether selected elements should be deleted from the source
	 * population.
	 * @param generator Random number generator to choose elements.
	 */
	SelectionNRandom(size_t selection, bool delete_selected, std::mt19937& generator);

	/**
	 * Randomly selects N inidiviuals.
	 * @param pop_source Population from which individuals are selected.
	 * @param pop_target Population to which the individuals are added. Population is cleared before
	 * adding individuals.
	 */
	void operator()(EoPopulation<EOT>& pop_source, EoPopulation<EOT>& pop_target);

private:
	size_t m_number_selection;
	bool m_delete_selected;
	std::mt19937& m_generator;
};


/**
 * Performs a tournament between a number of individuals resulting
 * in a winner with the higest fitness.
 * @tparam EOT Type of the chromosomes of the population.
 */
template <class EOT>
struct Tournament
{
	/**
	 * Constructor.
	 * @param contestants Number of contestants in each tournament.
	 * @param selection Number of tournaments.
	 * @param generator Random number generator to select contestants in tournament.
	 */
	Tournament(size_t contestants, size_t selection, std::mt19937& generator);

	/**
	 * Perform tournament with m_number_contestants contestants until a selection of
	 * m_number_selection winners is done.
	 * @param pop_source Source population from which individuals for tournament are chosen.
	 * @param pop_target Target population where winners of the tournaments are emplaced. Population
	 * is cleared before adding individuals.
	 */
	void operator()(EoPopulation<EOT>& pop_source, EoPopulation<EOT>& pop_target);

private:
	size_t m_number_contestants;
	size_t m_number_selection;
	SelectionNRandom<EOT> m_selector;
};


/**
 * Performs mating of the individuals of a population with a given rate.
 * @tparam EOT Type of the chromosomes of the population.
 */
template <class EOT>
struct MatingExchangeBlocks
{
	/**
	 * Constructor.
	 * @param chrom_size Size of the chromosomes of the individuals.
	 * @param block_size Size of the block of elements that is interchanged between mating partners.
	 * @param p_mate Probability of mating between two selected individuals.
	 * @param generator Random number generator for choice of mating partners.
	 */
	MatingExchangeBlocks(
	    size_t chrom_size, size_t block_size, double p_mate, std::mt19937& generator);

	/**
	 * Performs the mating of the individuals in the source population
	 * resulting in a new population. The mating happens with a given rate.
	 * Two maiting individuals exchange a block of contents.
	 * @param pop_source Source population for mating.
	 * @param pop_target Target population for mating. Population is cleared before
	 * adding individuals.
	 */
	void operator()(EoPopulation<EOT>& pop_source, EoPopulation<EOT>& pop_target);

private:
	size_t m_block_size;
	std::mt19937& m_generator;
	std::bernoulli_distribution m_mate_distribution;
	std::uniform_int_distribution<size_t> m_position;
	SelectionNRandom<EOT> m_selector;
};


/**
 * Peforms random bit flips on some individuals of a population.
 *  @tparam EOT Type of the chromosomes of the population.
 */
template <class EOT>
struct MutationBitFlip
{
	/**
	 * Constructor.
	 * @param p_mutate_individual Probability that an indidviual is mutated.
	 * @param p_mutate_gene Probability for each gene of a mutated individual to be changed.
	 * @param generator Random number generator to select individuals and genes for mutation.
	 */
	MutationBitFlip(double p_mutate_individual, double p_mutate_gene, std::mt19937& generator);

	/**
	 * Performs random bit flips on the given population.
	 * @param pop The population on which the mutation should be applied.
	 */
	void operator()(EoPopulation<EOT>& pop);

private:
	std::bernoulli_distribution m_mutate_individual;
	std::bernoulli_distribution m_mutate_gene;
	std::mt19937& m_generator;
};

/**
 * Shifts the bits of the individulas in a population to the right.
 * @tparam EOT type of the chromosomes of the population.
 */
template <class EOT>
struct MutationShiftRound
{
	/**
	 * Constructor.
	 * @param p_shift Probability to perform a shift to an individual.
	 * @param min_shift Lower limit for the shifting.
	 * @param max_shift Upper limit for the shifting.
	 * @param generator Random number generator to select individuals for shifting and determine
	 * shifting distance.
	 */
	MutationShiftRound(double p_shift, size_t min_shift, size_t max_shift, std::mt19937& generator);

	/**
	 * Shifts the bits of the individuals in the given population.
	 * @param pop Population on which shifting is applied.
	 */
	void operator()(EoPopulation<EOT>& pop);

private:
	/*Specific to representation of population. Interpretation as 2d grid.
	a b c
	d e f
	is represented as (a, d, e, b, c, f)
	Therfore shifting uneven numbers requires the chromosome to be "flipped".
	*/
	void mirror_chrom(EOT& chrom);

	std::bernoulli_distribution m_shift_distribution;
	std::uniform_int_distribution<int> m_shift_distance;
	std::mt19937& m_generator;
};

/**
 * For a 2d interpretation of the chromosome columns with default elements are added at a random
 * location. The chromosome is a 1d-vector with alternating rows.
 * For example (a, e, f, b, c, g , d, h) represents:
 * a b c d
 * e f g h
 * @tparam EOT type of the chromosomes of the population.
 */
template <class EOT>
struct MutationAddColumns
{
	/**
	 * Constructor.
	 * @param p_add Probability columns are added to an individual.
	 * @param number_columns Number of columns added to the chromosome of an individual.
	 * @param together Whether the columns are added as a block or individually.
	 * @param x_min Lowest column number where columns can be added.
	 * @param x_max Highest column number where columns can be added.
	 * @param generator Random number genertor to choose which individual is mutated and where the
	 * columns are inserted.
	 */
	MutationAddColumns(
	    double p_add,
	    size_t number_columns,
	    bool together,
	    size_t x_min,
	    size_t x_max,
	    std::mt19937& generator);
	/**
	 * Adds columns with default elements to random positions in the chromosome of randomly selected
	 * individuals of the given population.
	 * @param pop Population on which the columns are added.
	 */
	void operator()(EoPopulation<EOT>& pop);

private:
	bool m_together;
	size_t m_number_columns;
	std::bernoulli_distribution m_distribution;
	std::uniform_int_distribution<int> m_position;
	std::mt19937& m_generator;
};

/**
 * For a 2d interpretation of the chromosome randomly selected columns of randomly selected
 * individuals are removed. The chromosome is a 1d-vector with alternating rows.
 * For example (a, e, f, b, c, g , d, h) represents:
 * a b c d
 * e f g h
 * @tparam EOT type of the chromosomes of the population.
 */
template <class EOT>
struct MutationRemoveColumns
{
	/**
	 * Constructor.
	 * @param p_add Probability to remove columns in an individulas chromosome.
	 * @param number_columns Number of columns to be removed.
	 * @param together Whether a block of columns or individual columns are removed.
	 * @param x_min Lower limit in column number where columns can be removed.
	 * @param x_max Upper limit in column number where columns can be removed.
	 * @param generator Random number generator to choose which individuals have columns removed and
	 * the position of the removed columns.
	 */
	MutationRemoveColumns(
	    double p_add,
	    size_t number_columns,
	    bool together,
	    size_t x_min,
	    size_t x_max,
	    std::mt19937& generator);
	/**
	 * Removes columns and replaces them with columns with default elements at the end of the
	 * chromosome. Is applied to randomly selected individuals of the population.
	 * @param pop Population on which the columns are removed.
	 */
	void operator()(EoPopulation<EOT>& pop);

private:
	bool m_together;
	size_t m_number_columns;
	std::bernoulli_distribution m_distribution;
	std::uniform_int_distribution<int> m_position;
	std::mt19937& m_generator;
};

} // namespace grenade::vx::network::abstract

#include "grenade/vx/network/abstract/multicompartment_evolutionary_helper.tcc"