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

} // namespace grenade::vx::network::abstract