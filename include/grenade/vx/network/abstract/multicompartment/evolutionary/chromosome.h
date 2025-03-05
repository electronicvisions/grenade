#pragma once
#include <vector>

namespace grenade::vx::network::abstract {

template <class ValueT, class FitnessT>
struct EoChrom : public std::vector<ValueT>
{
	EoChrom();
	EoChrom(size_t number_genes);

	// Sets fitnss of chromosome
	void fitness(FitnessT const& value);
	FitnessT fitness() const;

	// Returns invalidity of fitness
	bool valid() const;

	// Comparison operator
	bool operator<(EoChrom<ValueT, FitnessT> const& other) const;
	bool operator>(EoChrom<ValueT, FitnessT> const& other) const;

private:
	FitnessT m_fitness;
	bool m_valid = false;
};

} // namespace grenade::vx::network::abstract

#include "grenade/vx/network/abstract/multicompartment/evolutionary/chromosome.tcc"