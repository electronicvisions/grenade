#include "grenade/vx/network/external_population.h"

#include <ostream>

namespace grenade::vx::network {

ExternalPopulation::ExternalPopulation(size_t const size) : size(size) {}

bool ExternalPopulation::operator==(ExternalPopulation const& other) const
{
	return size == other.size;
}

bool ExternalPopulation::operator!=(ExternalPopulation const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, ExternalPopulation const& population)
{
	os << "ExternalPopulation(size: " << population.size << ")";
	return os;
}

} // namespace grenade::vx::network
