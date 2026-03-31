#pragma once

#include "grenade/common/compartment_on_neuron.h"
#include "grenade/vx/network/abstract/multicompartment/placement/algorithm.h"

namespace grenade::vx::network::abstract {

struct NeuronPlacementResult
{
	AlgorithmResult result;
	Neuron neuron_build;
	std::map<grenade::common::CompartmentOnNeuron, NumberTopBottom> resources_build;
	NumberTopBottom resources_total_build;
	std::map<grenade::common::CompartmentOnNeuron, grenade::common::CompartmentOnNeuron>
	    descriptor_mapping;
};

} // namespace grenade::vx::network::abstract