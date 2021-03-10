#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/population.h"
#include <optional>

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace network {

/**
 * Hardware graph representation of a placed and routed network.
 */
struct GENPYBIND(visible) NetworkGraph
{
	/** Underlying network. */
	std::shared_ptr<Network> const network;

	/** Graph representing the network. */
	Graph const graph;

	/** Vertex descriptor at which to insert external spike data. */
	std::optional<Graph::vertex_descriptor> const event_input_vertex;

	/** Vertex descriptor from which to extract recorded spike data. */
	std::optional<Graph::vertex_descriptor> const event_output_vertex;

	/** Vertex descriptor from which to extract recorded madc sample data. */
	std::optional<Graph::vertex_descriptor> const madc_sample_output_vertex;

	/**
	 * Spike labels corresponding to each neuron in a population.
	 * For external populations these are the input spike labels, for internal population this is
	 * only given for populations with enabled recording.
	 */
	typedef std::map<PopulationDescriptor, std::vector<std::vector<haldls::vx::v2::SpikeLabel>>>
	    SpikeLabels;
	SpikeLabels spike_labels;
};

} // namespace network

} // namespace grenade::vx
