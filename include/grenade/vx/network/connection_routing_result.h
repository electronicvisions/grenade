#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/projection_on_network.h"
#include "hate/visibility.h"
#include <cstddef>
#include <iosfwd>
#include <map>
#include <vector>

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Translation of a single connection to a collection of hardware synapse routes to an atomic neuron
 * circuit of the logical neuron's target compartment.
 */
struct GENPYBIND(visible) ConnectionToHardwareRoutes
{
	/** Indices of atomic neurons in target compartment. */
	std::vector<size_t> atomic_neurons_on_target_compartment;

	bool operator==(ConnectionToHardwareRoutes const& other) const SYMBOL_VISIBLE;
	bool operator!=(ConnectionToHardwareRoutes const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, ConnectionToHardwareRoutes const& routes)
	    SYMBOL_VISIBLE;
};


/**
 * Translation between synapses between logical neuron compartments and synapses between atomic
 * neurons within compartments.
 * Contains a translation for each connection of each projection.
 * The order of the translations matches the order of the connections in the projection.
 */
typedef std::map<ProjectionOnExecutionInstance, std::vector<ConnectionToHardwareRoutes>>
    ConnectionRoutingResult;

} // namespace grenade::vx::network
