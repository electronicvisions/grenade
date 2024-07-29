#include "grenade/vx/network/inter_execution_instance_projection.h"

#include "grenade/vx/network/projection.h"
#include <ostream>

namespace grenade::vx::network {

InterExecutionInstanceProjection::Connection::Connection(
    Index const& index_pre, Index const& index_post, common::Time const& delay) :
    index_pre(index_pre), index_post(index_post), delay(delay)
{}

bool InterExecutionInstanceProjection::Connection::operator==(Connection const& other) const
{
	return index_pre == other.index_pre && index_post == other.index_post && delay == other.delay;
}

bool InterExecutionInstanceProjection::Connection::operator!=(Connection const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(
    std::ostream& os, InterExecutionInstanceProjection::Connection const& connection)
{
	os << "Connection(" << connection.index_pre << " -> " << connection.index_post
	   << ", delay: " << connection.delay << ")";
	return os;
}


InterExecutionInstanceProjection::InterExecutionInstanceProjection(
    Connections const& connections,
    PopulationOnNetwork const population_pre,
    PopulationOnNetwork const population_post) :
    connections(connections), population_pre(population_pre), population_post(population_post)
{}

InterExecutionInstanceProjection::InterExecutionInstanceProjection(
    Connections&& connections,
    PopulationOnNetwork const population_pre,
    PopulationOnNetwork const population_post) :
    connections(std::move(connections)),
    population_pre(population_pre),
    population_post(population_post)
{}

bool InterExecutionInstanceProjection::operator==(
    InterExecutionInstanceProjection const& other) const
{
	return connections == other.connections && population_pre == other.population_pre &&
	       population_post == other.population_post;
}

bool InterExecutionInstanceProjection::operator!=(
    InterExecutionInstanceProjection const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, InterExecutionInstanceProjection const& projection)
{
	os << "InterExecutionInstanceProjection(\n";
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
