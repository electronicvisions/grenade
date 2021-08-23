#include "grenade/vx/network/madc_recording.h"

namespace grenade::vx::network {

MADCRecording::MADCRecording(
    PopulationDescriptor const population, size_t const index, Source const source) :
    population(population), index(index), source(source)
{}

bool MADCRecording::operator==(MADCRecording const& other) const
{
	return population == other.population && index == other.index && source == other.source;
}

bool MADCRecording::operator!=(MADCRecording const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::network
