#include "grenade/vx/network/projection.h"

#include <ostream>

namespace grenade::vx::network {

std::ostream& operator<<(std::ostream& os, Projection::Connection::Index const& index)
{
	os << "Index(" << index.first << ", " << index.second << ")";
	return os;
}

Projection::Connection::Connection(
    Index const& index_pre, Index const& index_post, Weight const weight) :
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

std::ostream& operator<<(std::ostream& os, Projection::Connection const& connection)
{
	os << "Connection(" << connection.index_pre << " -> " << connection.index_post
	   << ", weight: " << connection.weight << ")";
	return os;
}


Projection::Projection(
    Receptor const& receptor,
    Connections const& connections,
    PopulationOnExecutionInstance const population_pre,
    PopulationOnExecutionInstance const population_post,
    common::EntityOnChip::ChipOnExecutor const chip_on_executor) :
    common::EntityOnChip(chip_on_executor),
    receptor(receptor),
    connections(connections),
    population_pre(population_pre),
    population_post(population_post)
{}

Projection::Projection(
    Receptor const& receptor,
    Connections&& connections,
    PopulationOnExecutionInstance const population_pre,
    PopulationOnExecutionInstance const population_post,
    common::EntityOnChip::ChipOnExecutor const chip_on_executor) :
    common::EntityOnChip(chip_on_executor),
    receptor(receptor),
    connections(std::move(connections)),
    population_pre(population_pre),
    population_post(population_post)
{}

bool Projection::operator==(Projection const& other) const
{
	return receptor == other.receptor && connections == other.connections &&
	       population_pre == other.population_pre && population_post == other.population_post &&
	       static_cast<common::EntityOnChip const&>(*this) ==
	           static_cast<common::EntityOnChip const&>(other);
}

bool Projection::operator!=(Projection const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, Projection const& projection)
{
	os << "Projection(\n";
	os << "\t" << static_cast<common::EntityOnChip const&>(projection) << "\n";
	os << "\treceptor: " << projection.receptor << "\n";
	os << "\tpopulation_pre: " << projection.population_pre << "\n";
	os << "\tpopulation_post: " << projection.population_post << "\n";
	os << "\tconnections:\n";
	for (auto const& connection : projection.connections) {
		os << "\t\t" << connection << "\n";
	}
	os << ")";
	return os;
}

} // namespace grenade::vx::network
