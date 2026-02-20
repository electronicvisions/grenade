#pragma once

#include "grenade/vx/network/abstract/multicompartment/evolutionary/population.h"
#include <algorithm>
#include <stdexcept>

namespace grenade::vx::network::abstract {

template <class EoChromT>
EoPopulation<EoChromT> EoPopulation<EoChromT>::sort_ascending() const
{
	EoPopulation<EoChromT> result;
	if (result != *this) {
		result = *this;
	}
	std::sort(result.begin(), result.end(), std::less<EoChromT>());
	return result;
}

template <class EoChromT>
EoPopulation<EoChromT> EoPopulation<EoChromT>::sort_descending() const
{
	EoPopulation<EoChromT> result;
	if (result != *this) {
		result = *this;
	}
	std::sort(result.begin(), result.end(), std::greater<EoChromT>());
	return result;
}

template <class EoChromT>
EoChromT EoPopulation<EoChromT>::get_best_individual()
{
	EoChromT best_chrom = front();
	for (auto chrom : *this) {
		if (!chrom.valid()) {
			throw std::logic_error("Chromosome with invalid fitness in population.");
		}
		if (chrom > best_chrom) {
			best_chrom = chrom;
		}
	}
	return best_chrom;
}

} // namespace grenade::vx::network::abstract