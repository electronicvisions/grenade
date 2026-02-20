#pragma once
#include "grenade/vx/network/abstract/multicompartment/evolutionary/helper.h"

#include <stdexcept>

namespace grenade::vx::network::abstract {

template <class EOT>
SelectionNBest<EOT>::SelectionNBest(size_t number, bool delete_selected) :
    m_number_selection(number), m_delete_selected(delete_selected)
{
}

template <class EOT>
void SelectionNBest<EOT>::operator()(EoPopulation<EOT>& pop_source, EoPopulation<EOT>& pop_target)
{
	if (&pop_source == &pop_target) {
		throw std::logic_error("Cannot use source population as target population for selection.");
	}
	if (pop_source.size() <= m_number_selection) {
		std::stringstream ss;
		ss << "Cannot select " << m_number_selection << " individuals from population with "
		   << pop_source.size() << "individuals.";
		throw std::logic_error(ss.str());
	}

	pop_source = std::move(pop_source.sort_ascending());
	pop_target.clear();

	if (m_delete_selected) {
		pop_target.insert(
		    pop_target.begin(), std::make_move_iterator(pop_source.begin()),
		    std::make_move_iterator(pop_source.begin() + m_number_selection));
		pop_source.erase(pop_source.begin(), pop_source.begin() + m_number_selection);
	} else {
		pop_target.insert(
		    pop_target.begin(), pop_source.begin(), pop_source.begin() + m_number_selection);
	}
}

template <class EOT>
SelectionNRandom<EOT>::SelectionNRandom(
    size_t selection, bool delete_selected, std::mt19937& generator) :
    m_number_selection(selection), m_delete_selected(delete_selected), m_generator(generator)
{
}

template <class EOT>
void SelectionNRandom<EOT>::operator()(EoPopulation<EOT>& pop_source, EoPopulation<EOT>& pop_target)
{
	std::uniform_int_distribution<size_t> selector(0, pop_source.size() - 1);
	pop_target.clear();

	if (m_delete_selected) {
		if (pop_source.size() < m_number_selection) {
			std::stringstream ss;
			ss << "Cannot select " << m_number_selection << " individuals. Population has "
			   << pop_source.size() << " individuals.";
			throw std::range_error(ss.str());
		} else if (pop_source.size() == m_number_selection) {
			pop_target = pop_source;
		}
	}

	// Generate N random positions in the range of the population.
	std::set<size_t> positions;
	while (positions.size() < m_number_selection) {
		size_t position = selector(m_generator);
		positions.emplace(position);
	}

	// Add N individuals to selection.
	for (auto position : positions) {
		pop_target.push_back(pop_source[position]);
	}

	// Remove N individuals from source population.
	if (m_delete_selected) {
		while (!positions.empty()) {
			pop_source.erase(pop_source.begin() + *(--positions.end()));
			positions.erase(--positions.end());
		}
	}
}

template <class EOT>
Tournament<EOT>::Tournament(size_t contestants, size_t selection, std::mt19937& generator) :
    m_number_contestants(contestants),
    m_number_selection(selection),
    m_selector(contestants, false, generator)
{
}

template <class EOT>
void Tournament<EOT>::operator()(EoPopulation<EOT>& pop_source, EoPopulation<EOT>& pop_target)
{
	if (&pop_source == &pop_target) {
		throw std::logic_error("Cannot use source population as target population for tournament.");
	}

	EoPopulation<EOT> selection;
	EoPopulation<EOT> selection_descending;

	pop_target.resize(m_number_selection);
	for (size_t i = 0; i < m_number_selection; i++) {
		m_selector(pop_source, selection);
		selection_descending = std::move(selection.sort_descending());
		pop_target[i] = selection_descending.front();
	}
}

template <class EOT>
MatingExchangeBlocks<EOT>::MatingExchangeBlocks(
    size_t chrom_size, size_t block_size, double p_mate, std::mt19937& generator) :
    m_block_size(block_size),
    m_generator(generator),
    m_mate_distribution(p_mate),
    m_position(0, chrom_size),
    m_selector(2, true, generator)
{
}

template <class EOT>
void MatingExchangeBlocks<EOT>::operator()(
    EoPopulation<EOT>& pop_source, EoPopulation<EOT>& pop_target)
{
	if (&pop_source == &pop_target) {
		throw std::logic_error("Cannot use source population as target population for mating.");
	}

	EoPopulation<EOT> mates;

	// Match size of target population with size of source population.
	pop_target.resize(pop_source.size());

	for (size_t i = 0; i < pop_target.size() - 1; i += 2) {
		m_selector(pop_source, mates);

		// Store content of first mating partner.
		EOT tmp = mates.front();

		// Distributed crossover
		if (m_mate_distribution(m_generator)) {
			size_t block_start = m_position(m_generator);
			size_t block_end = block_start + m_block_size;
			if (block_end > mates.front().size()) {
				block_end = mates.front().size();
			}

			// Swap contents inside of selected block.
			for (size_t j = block_start; j < block_end; j++) {
				tmp[j] = mates.back()[j];
				mates.back()[j] = mates.front()[j];
			}

			// Assign tmp to first mate
			mates.front() = tmp;
		}

		// Insert crossed or uncrossed mates into population
		pop_target[i] = mates.front();
		pop_target[i + 1] = mates.back();
	}
}

template <class EOT>
MutationBitFlip<EOT>::MutationBitFlip(
    double p_mutate_individual, double p_mutate_gene, std::mt19937& generator) :
    m_mutate_individual(p_mutate_individual), m_mutate_gene(p_mutate_gene), m_generator(generator)
{
}

template <class EOT>
void MutationBitFlip<EOT>::operator()(EoPopulation<EOT>& pop)
{
	for (auto& chrom : pop) {
		if (m_mutate_individual(m_generator)) {
			for (size_t i = 0; i < chrom.size(); i++) {
				if (m_mutate_gene(m_generator))
					chrom[i] = !chrom[i];
			}
		}
	}
}

template <class EOT>
MutationShiftRound<EOT>::MutationShiftRound(
    double p_shift, size_t min_shift, size_t max_shift, std::mt19937& generator) :
    m_shift_distribution(p_shift), m_shift_distance(min_shift, max_shift), m_generator(generator)
{
}

template <class EOT>
void MutationShiftRound<EOT>::operator()(EoPopulation<EOT>& pop)
{
	for (EOT& chrom : pop) {
		auto shift_distance = get_random_shift();
		if (!shift_distance) {
			continue;
		}
		shift(chrom, *shift_distance);
	}
}

template <class EOT>
std::optional<size_t> MutationShiftRound<EOT>::get_random_shift()
{
	if (!m_shift_distribution(m_generator)) {
		return std::nullopt;
	}

	return std::make_optional(m_shift_distance(m_generator));
}

template <class EOT>
void MutationShiftRound<EOT>::shift(EOT& chrom, size_t x_shift)
{
	// If uneven number of circuits is shifted the bit string needs to be mirrored
	if (x_shift % 2 != 0) {
		mirror_chrom(chrom);
	}

	// Number of shifted bits
	size_t bit_shift = (9 * x_shift) % chrom.size();

	std::rotate(begin(chrom), begin(chrom) + bit_shift, end(chrom));
}

template <class EOT>
void MutationShiftRound<EOT>::mirror_chrom(EOT& chrom)
{
	EOT temp_chrom = chrom;
	chrom.clear();
	// Iterate over all collumns
	for (size_t collumn = 0; collumn < temp_chrom.size() / 9; collumn++) {
		// Inserts switch between rows first.
		chrom.push_back(temp_chrom.at(9 * collumn));
		// Insert second part of chromosom first.
		for (size_t j = 9 * collumn + 5; j < 9 * collumn + 9; j++) {
			chrom.push_back(temp_chrom.at(j));
		}
		// Insert first part of chromosom second.
		for (size_t j = 9 * collumn + 1; j < 9 * collumn + 5; j++) {
			chrom.push_back(temp_chrom.at(j));
		}
	}
}

template <class EOT>
MutationAddColumns<EOT>::MutationAddColumns(
    double p_add,
    size_t number_columns,
    bool together,
    size_t x_min,
    size_t x_max,
    std::mt19937& generator) :
    m_together(together),
    m_number_columns(number_columns),
    m_distribution(p_add),
    m_position(x_min, x_max),
    m_generator(generator)
{
}

template <class EOT>
void MutationAddColumns<EOT>::operator()(EoPopulation<EOT>& pop)
{
	for (auto& chrom : pop) {
		auto x_positions = get_random_add();
		if (!x_positions) {
			continue;
		}
		add_columns(chrom, *x_positions);
	}
}

template <class EOT>
std::optional<std::vector<size_t>> MutationAddColumns<EOT>::get_random_add()
{
	if (!m_distribution(m_generator)) {
		return std::nullopt;
	}

	std::vector<size_t> x_modify;

	// Place #m_number_columns inidividual columns in genome of individuals
	if (!m_together) {
		for (size_t i = 0; i < m_number_columns; i++) {
			// Get insertion point
			x_modify.push_back(m_position(m_generator));
		}
	}
	// Place block of size #m_number_columns in genome of individuals
	else {
		// Get insertion point
		size_t x_insert = m_position(m_generator);
		for (size_t i = x_insert; i < x_insert + m_number_columns; i++) {
			x_modify.push_back(i);
		}
	}
	return std::make_optional(x_modify);
}

template <class EOT>
void MutationAddColumns<EOT>::add_columns(EOT& chrom, std::vector<size_t> x_positions)
{
	for (auto x_modify : x_positions) {
		std::shift_right(begin(chrom) + 9 * x_modify, end(chrom), 9 * 1);

		// Fill new space with zeros
		for (size_t j = 9 * x_modify; j < std::min(9 * (x_modify + 1), chrom.size()); j++) {
			chrom[j] = 0;
		}
	}
}

template <class EOT>
MutationRemoveColumns<EOT>::MutationRemoveColumns(
    double p_add,
    size_t number_columns,
    bool together,
    size_t x_min,
    size_t x_max,
    std::mt19937& generator) :
    m_together(together),
    m_number_columns(number_columns),
    m_distribution(p_add),
    m_position(x_min, x_max),
    m_generator(generator)
{
}

template <class EOT>
void MutationRemoveColumns<EOT>::operator()(EoPopulation<EOT>& pop)
{
	for (auto& chrom : pop) {
		std::optional<std::vector<size_t>> x_positions = get_random_remove();
		if (!x_positions) {
			continue;
		}
		remove_columns(chrom, *x_positions);
	}
}

template <class EOT>
std::optional<std::vector<size_t>> MutationRemoveColumns<EOT>::get_random_remove()
{
	if (!m_distribution(m_generator)) {
		return std::nullopt;
	}

	std::vector<size_t> x_modify;

	// Remove #m_number_columns individual columns in genome of individuals
	if (!m_together) {
		for (size_t i = 0; i < m_number_columns; i++) {
			// Get insertion point
			x_modify.push_back(m_position(m_generator));
		}
	}
	// Remove block of size #m_number_columns in genome of individuals
	else {
		// Get insertion point
		size_t x_remove = m_position(m_generator);
		for (size_t i = x_remove; i < x_remove + m_number_columns; i++) {
			x_modify.push_back(i);
		}
	}
	return std::make_optional(x_modify);
}

template <class EOT>
void MutationRemoveColumns<EOT>::remove_columns(EOT& chrom, std::vector<size_t> x_positions)
{
	for (auto x_modify : x_positions) {
		std::shift_left(begin(chrom) + 9 * x_modify, end(chrom), 9 * 1);

		// Fill new space at end of chrom with zeros
		for (size_t j = chrom.size() - 9 * 1; j < chrom.size(); j++) {
			chrom[j] = 0;
		}
	}
}


} // namespace grenade::vx::network::abstract