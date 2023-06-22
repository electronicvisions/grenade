#include "grenade/vx/network/external_source_population.h"

#include <ostream>

namespace grenade::vx::network {

ExternalSourcePopulation::ExternalSourcePopulation(size_t const size) : size(size) {}

bool ExternalSourcePopulation::operator==(ExternalSourcePopulation const& other) const
{
	return size == other.size;
}

bool ExternalSourcePopulation::operator!=(ExternalSourcePopulation const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, ExternalSourcePopulation const& population)
{
	os << "ExternalSourcePopulation(size: " << population.size << ")";
	return os;
}

} // namespace grenade::vx::network
