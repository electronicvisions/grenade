#pragma once
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/genpybind.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/padi.h"
#include "hate/visibility.h"
#include <chrono>
#include <iosfwd>
#include <map>
#include <set>
#include <vector>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Result of placement.
 */
struct GENPYBIND(visible) PlacementResult
{
	struct Chip
	{
		// Mapping from neuron indices per population to atomic neurons.
		typedef std::map<
		    grenade::common::VertexOnTopology,
		    std::map<size_t, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>>
		    NeuronAnchors;

		NeuronAnchors neuron_anchors;

		typedef std::map<
		    grenade::common::VertexOnTopology,
		    std::map<
		        halco::hicann_dls::vx::v3::HemisphereOnDLS,
		        halco::hicann_dls::vx::v3::PADIBusOnPADIBusBlock>>
		    BackgroundLocations;

		BackgroundLocations background_locations;

		std::set<grenade::common::VertexOnTopology> external_sources;
	};
	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::map<grenade::vx::common::ChipOnConnection, Chip>>
	    chips;

	struct TimingStatistics
	{
		std::chrono::microseconds placement;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream&, PlacementResult::TimingStatistics const&)
		    SYMBOL_VISIBLE;
	} timing_statistics;

	PlacementResult() = default;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream&, PlacementResult const&) SYMBOL_VISIBLE;
};

} // namespace abstract
} // namespace grenade::vx::abstract
