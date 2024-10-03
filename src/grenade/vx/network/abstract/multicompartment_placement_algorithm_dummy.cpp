#include "grenade/vx/network/abstract/multicompartment_placement_algorithm_dummy.h"

namespace grenade::vx::network::abstract {


PlacementAlgorithmDummy::PlacementAlgorithmDummy(CoordinateSystem const& configuration) :
    coordinate_system(configuration)
{
}


AlgorithmResult PlacementAlgorithmDummy::run(
    CoordinateSystem const& configuration_start, Neuron const&, ResourceManager const&)
{
	AlgorithmResult result;

	result.coordinate_system = configuration_start;
	result.finished = true;

	return result;
}

std::unique_ptr<PlacementAlgorithm> PlacementAlgorithmDummy::clone() const
{
	auto new_algorithm = std::make_unique<PlacementAlgorithmDummy>();
	return new_algorithm;
}

void PlacementAlgorithmDummy::reset()
{
	coordinate_system = CoordinateSystem();
}

} // namespace grenade::vx::network::abstract