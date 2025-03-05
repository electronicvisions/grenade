#pragma once

#include "grenade/vx/network/abstract/multicompartment/placement/algorithm.h"

namespace grenade::vx::network::abstract {

struct PlacementResult
{
	AlgorithmResult result;
	Neuron neuron_build;
	std::map<CompartmentOnNeuron, NumberTopBottom> resources_build;
	NumberTopBottom resources_total_build;
	std::map<CompartmentOnNeuron, CompartmentOnNeuron> descriptor_mapping;
};

} // namespace grenade::vx::network::abstract