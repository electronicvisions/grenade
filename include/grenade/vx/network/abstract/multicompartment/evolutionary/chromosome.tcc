#pragma once
#include "grenade/vx/network/abstract/multicompartment/evolutionary/chromosome.h"

namespace grenade::vx::network::abstract {

template <class ValueT, class FitnessT>
EoChrom<ValueT, FitnessT>::EoChrom()
{
}

template <class ValueT, class FitnessT>
EoChrom<ValueT, FitnessT>::EoChrom(size_t number_genes)
{
	this->resize(number_genes);
}

template <class ValueT, class FitnessT>
void EoChrom<ValueT, FitnessT>::fitness(FitnessT const& fitness)
{
	m_fitness = fitness;
	m_valid = true;
}

template <class ValueT, class FitnessT>
FitnessT EoChrom<ValueT, FitnessT>::fitness() const
{
	return m_fitness;
}

template <class ValueT, class FitnessT>
bool EoChrom<ValueT, FitnessT>::valid() const
{
	return m_valid;
}

template <class ValueT, class FitnessT>
bool EoChrom<ValueT, FitnessT>::operator<(EoChrom<ValueT, FitnessT> const& other) const
{
	return m_fitness < other.m_fitness;
}

template <class ValueT, class FitnessT>
bool EoChrom<ValueT, FitnessT>::operator>(EoChrom<ValueT, FitnessT> const& other) const
{
	return m_fitness > other.m_fitness;
}


} // namespace grenade::vx::network::abstract