#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "halco/hicann-dls/vx/v2/synapse.h"
#include "halco/hicann-dls/vx/v2/synapse_driver.h"
#include "haldls/vx/v2/event.h"
#include "haldls/vx/v2/neuron.h"
#include "haldls/vx/v2/synapse_driver.h"
#include "hate/visibility.h"
#include "lola/vx/v2/synapse.h"
#include <iosfwd>
#include <map>
#include <vector>

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace network {

/**
 * Result of connection routing.
 */
struct GENPYBIND(visible) RoutingResult
{
	/**
	 * Placed single synapse connections with a N-to-one correspondence to an unplaced
	 * `Projection::Connection` with equal indexing.
	 */
	struct PlacedConnection
	{
		lola::vx::v2::SynapseMatrix::Weight const weight;
		lola::vx::v2::SynapseMatrix::Label const label;
		halco::hicann_dls::vx::v2::SynapseRowOnDLS const synapse_row;
		halco::hicann_dls::vx::v2::SynapseOnSynapseRow const synapse_on_row;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream&, PlacedConnection const&) SYMBOL_VISIBLE;
	};
	typedef std::map<ProjectionDescriptor, std::vector<std::vector<PlacedConnection>>> Connections;
	Connections connections;

	/**
	 * Spike label corresponding to each neuron in a external population.
	 */
	typedef std::map<PopulationDescriptor, std::vector<std::vector<haldls::vx::v2::SpikeLabel>>>
	    ExternalSpikeLabels;
	ExternalSpikeLabels external_spike_labels;

	/**
	 * Neuron event output address corresponding to each neuron in a on-chip population.
	 */
	typedef std::
	    map<PopulationDescriptor, std::vector<haldls::vx::v2::NeuronBackendConfig::AddressOut>>
	        InternalNeuronLabels;
	InternalNeuronLabels internal_neuron_labels;

	/**
	 * Synapse driver row address compare mask resulting from routing to be applied.
	 * This map is only required to feature used synapse drivers, all other will be
	 * considered disabled.
	 */
	typedef std::map<
	    halco::hicann_dls::vx::v2::SynapseDriverOnDLS,
	    haldls::vx::v2::SynapseDriverConfig::RowAddressCompareMask>
	    SynapseDriverRowAddressCompareMasks;
	SynapseDriverRowAddressCompareMasks synapse_driver_compare_masks;

	/**
	 * Synapse row mode resulting from routing to be applied.
	 * This map is only required to feature used synapse rows, all other will be considered
	 * disabled.
	 */
	typedef std::map<
	    halco::hicann_dls::vx::v2::SynapseRowOnDLS,
	    haldls::vx::v2::SynapseDriverConfig::RowMode>
	    SynapseRowModes;
	SynapseRowModes synapse_row_modes;

	/**
	 * Crossbar node configuration resulting from routing to be applied.
	 * This map is only required to feature used crossbar nodes, all other will be
	 * considered disabled.
	 */
	typedef std::map<halco::hicann_dls::vx::v2::CrossbarNodeOnDLS, haldls::vx::v2::CrossbarNode>
	    CrossbarNodes;
	CrossbarNodes crossbar_nodes;

	RoutingResult() = default;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream&, RoutingResult const&) SYMBOL_VISIBLE;
};

} // namespace network

} // namespace grenade::vx
