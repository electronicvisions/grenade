#pragma once
#include "grenade/vx/network/placed_logical/projection.h"
#include <cstddef>
#include <map>
#include <vector>

namespace grenade::vx::network::placed_logical {

/**
 * Translation of a single connection to a collection of hardware synapse routes to an atomic neuron
 * circuit of the logical neuron's target compartment.
 */
struct ConnectionToHardwareRoutes
{
	/** Indices of atomic neurons in target compartment. */
	std::vector<size_t> atomic_neurons_on_target_compartment;
};


/**
 * Translation between synapses between logical neuron compartments and synapses between atomic
 * neurons within compartments.
 * Contains a translation for each connection of each projection.
 * The order of the translations matches the order of the connections in the projection.
 */
typedef std::map<ProjectionDescriptor, std::vector<ConnectionToHardwareRoutes>>
    ConnectionRoutingResult;

} // namespace grenade::vx::network::placed_logical
