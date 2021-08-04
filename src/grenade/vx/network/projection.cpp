#include "grenade/vx/network/projection.h"

namespace grenade::vx::network {

Projection::Connection::Connection(
    size_t const index_pre, size_t const index_post, Weight const weight) :
    index_pre(index_pre), index_post(index_post), weight(weight)
{}

bool Projection::Connection::operator==(Connection const& other) const
{
	return index_pre == other.index_pre && index_post == other.index_post && weight == other.weight;
}

bool Projection::Connection::operator!=(Connection const& other) const
{
	return !(*this == other);
}


Projection::Projection(
    ReceptorType const receptor_type,
    Connections const& connections,
    PopulationDescriptor const population_pre,
    PopulationDescriptor const population_post) :
    receptor_type(receptor_type),
    connections(connections),
    population_pre(population_pre),
    population_post(population_post)
{}

Projection::Projection(
    ReceptorType const receptor_type,
    Connections&& connections,
    PopulationDescriptor const population_pre,
    PopulationDescriptor const population_post) :
    receptor_type(receptor_type),
    connections(std::move(connections)),
    population_pre(population_pre),
    population_post(population_post)
{}

bool Projection::operator==(Projection const& other) const
{
	return receptor_type == other.receptor_type && connections == other.connections &&
	       population_pre == other.population_pre && population_post == other.population_post;
}

bool Projection::operator!=(Projection const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::network
