#pragma once
#include "grenade/common/compartment_on_neuron.h"
#include <vector>

namespace grenade::vx::network::abstract {

/**
 * Neighbors of a compartment on a neuron, grouped in different types.
 *
 * Branches contain at least one compartment with more than two connections.
 * Chains contain compartments with two connections and one leaf at its end.
 * Leafs are single compartents with one connection.
 */
struct CompartmentNeighbours
{
	std::vector<grenade::common::CompartmentOnNeuron> branches;
	std::vector<grenade::common::CompartmentOnNeuron> chains;
	std::vector<grenade::common::CompartmentOnNeuron> leafs;

	size_t total()
	{
		return branches.size() + chains.size() + leafs.size();
	}
};


} // namespace grenade::vx::network::abstract