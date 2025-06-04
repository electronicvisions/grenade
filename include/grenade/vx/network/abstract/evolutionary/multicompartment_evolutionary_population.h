#pragma once
#include <vector>

namespace grenade::vx::network::abstract {

template <class EoChromT>
struct EoPopulation : public std::vector<EoChromT>
{
	using std::vector<EoChromT>::begin;
	using std::vector<EoChromT>::end;
	using std::vector<EoChromT>::front;
	using std::vector<EoChromT>::back;

	// Sorts individuals by their fitness.
	EoPopulation sort_ascending() const;
	EoPopulation sort_descending() const;

	// Returns element with highest fitness.
	EoChromT get_best_individual();
};

} // namespace grenade::vx::network::abstract

#include "grenade/vx/network/abstract/evolutionary/multicompartment_evolutionary_population.tcc"