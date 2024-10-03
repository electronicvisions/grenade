#pragma once
#include "grenade/vx/network/abstract/multicompartment_compartment_on_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_placement_coordinate_system.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) AlgorithmResult
{
	CoordinateSystem coordinate_system;
	std::vector<CompartmentOnNeuron> placed_compartments;
	bool finished = false;
};

} // namespace abstract
} // namespace grenade::vx::network