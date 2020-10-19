#include "grenade/vx/network/projection.h"

namespace grenade::vx::network {

Projection::Connection::Connection(
    size_t const index_pre,
    size_t const index_post,
    lola::vx::v2::SynapseMatrix::Weight const weight) :
    index_pre(index_pre), index_post(index_post), weight(weight)
{}

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

} // namespace grenade::vx::network
