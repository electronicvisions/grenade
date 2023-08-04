#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/connection_routing_result.h"
#include "grenade/vx/network/population_on_network.h"
#include "grenade/vx/network/projection_on_network.h"
#include "halco/hicann-dls/vx/v3/event.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "halco/hicann-dls/vx/v3/synapse_driver.h"
#include "haldls/vx/v3/background.h"
#include "haldls/vx/v3/event.h"
#include "haldls/vx/v3/neuron.h"
#include "haldls/vx/v3/routing_crossbar.h"
#include "haldls/vx/v3/synapse_driver.h"
#include "hate/visibility.h"
#include "lola/vx/v3/synapse.h"
#include <chrono>
#include <iosfwd>
#include <map>
#include <vector>

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Result of connection routing.
 */
struct GENPYBIND(visible) RoutingResult
{
	ConnectionRoutingResult connection_routing_result;

	/**
	 * Placed single synapse connections with a N-to-one correspondence to an unplaced
	 * `Projection::Connection` with equal indexing.
	 */
	struct PlacedConnection
	{
		lola::vx::v3::SynapseMatrix::Weight weight;
		lola::vx::v3::SynapseMatrix::Label label;
		halco::hicann_dls::vx::v3::SynapseRowOnDLS synapse_row;
		halco::hicann_dls::vx::v3::SynapseOnSynapseRow synapse_on_row;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream&, PlacedConnection const&) SYMBOL_VISIBLE;
	};
	typedef std::map<ProjectionOnNetwork, std::vector<std::vector<PlacedConnection>>> Connections;
	Connections connections;

	/**
	 * Spike label corresponding to each neuron in a external population.
	 */
	typedef std::
	    map<PopulationOnNetwork, std::vector<std::vector<halco::hicann_dls::vx::v3::SpikeLabel>>>
	        ExternalSpikeLabels;
	ExternalSpikeLabels external_spike_labels;

	/**
	 * Spike label corresponding to each neuron in a background source population.
	 */
	typedef std::map<
	    PopulationOnNetwork,
	    std::map<
	        halco::hicann_dls::vx::v3::HemisphereOnDLS,
	        std::vector<halco::hicann_dls::vx::v3::SpikeLabel>>>
	    BackgroundSpikeSourceLabels;
	BackgroundSpikeSourceLabels background_spike_source_labels;

	/**
	 * Background spike source mask for each population.
	 */
	typedef std::map<
	    PopulationOnNetwork,
	    std::map<
	        halco::hicann_dls::vx::v3::HemisphereOnDLS,
	        haldls::vx::v3::BackgroundSpikeSource::Mask>>
	    BackgroundSpikeSourceMasks;
	BackgroundSpikeSourceMasks background_spike_source_masks;

	/**
	 * Neuron event output address corresponding to each neuron in a on-chip population.
	 */
	typedef std::map<
	    PopulationOnNetwork,
	    std::vector<std::map<
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
	        std::vector<std::optional<haldls::vx::v3::NeuronBackendConfig::AddressOut>>>>>
	    InternalNeuronLabels;
	InternalNeuronLabels internal_neuron_labels;

	/**
	 * Synapse driver row address compare mask resulting from routing to be applied.
	 * This map is only required to feature used synapse drivers, all other will be
	 * considered disabled.
	 */
	typedef std::map<
	    halco::hicann_dls::vx::v3::SynapseDriverOnDLS,
	    haldls::vx::v3::SynapseDriverConfig::RowAddressCompareMask>
	    SynapseDriverRowAddressCompareMasks;
	SynapseDriverRowAddressCompareMasks synapse_driver_compare_masks;

	/**
	 * Synapse row mode resulting from routing to be applied.
	 * This map is only required to feature used synapse rows, all other will be considered
	 * disabled.
	 */
	typedef std::map<
	    halco::hicann_dls::vx::v3::SynapseRowOnDLS,
	    haldls::vx::v3::SynapseDriverConfig::RowMode>
	    SynapseRowModes;
	SynapseRowModes synapse_row_modes;

	/**
	 * Crossbar node configuration resulting from routing to be applied.
	 * This map is only required to feature used crossbar nodes, all other will be
	 * considered disabled.
	 */
	typedef std::map<halco::hicann_dls::vx::v3::CrossbarNodeOnDLS, haldls::vx::v3::CrossbarNode>
	    CrossbarNodes;
	CrossbarNodes crossbar_nodes;

	struct TimingStatistics
	{
		std::chrono::microseconds routing;
	} timing_statistics;

	RoutingResult() = default;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream&, RoutingResult const&) SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network
