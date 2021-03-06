#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/population.h"
#include "hate/visibility.h"
#include <optional>

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace network {

struct RoutingResult;
struct NetworkGraph;

NetworkGraph build_network_graph(
    std::shared_ptr<Network> const& network, RoutingResult const& routing_result);

/**
 * Hardware graph representation of a placed and routed network.
 */
struct GENPYBIND(visible) NetworkGraph
{
	NetworkGraph() = default;

	/** Underlying network. */
	GENPYBIND(getter_for(network))
	std::shared_ptr<Network> const& get_network() const SYMBOL_VISIBLE;

	/** Graph representing the network. */
	GENPYBIND(getter_for(graph))
	Graph const& get_graph() const SYMBOL_VISIBLE;

	/** Vertex descriptor at which to insert external spike data. */
	GENPYBIND(getter_for(event_input_vertex))
	std::optional<Graph::vertex_descriptor> get_event_input_vertex() const SYMBOL_VISIBLE;

	/** Vertex descriptor from which to extract recorded spike data. */
	GENPYBIND(getter_for(event_output_vertex))
	std::optional<Graph::vertex_descriptor> get_event_output_vertex() const SYMBOL_VISIBLE;

	/** Vertex descriptor from which to extract recorded madc sample data. */
	GENPYBIND(getter_for(madc_sample_output_vertex))
	std::optional<Graph::vertex_descriptor> get_madc_sample_output_vertex() const SYMBOL_VISIBLE;

	/**
	 * Spike labels corresponding to each neuron in a population.
	 * For external populations these are the input spike labels, for internal population this is
	 * only given for populations with enabled recording.
	 */
	typedef std::map<PopulationDescriptor, std::vector<std::vector<haldls::vx::v2::SpikeLabel>>>
	    SpikeLabels;
	GENPYBIND(getter_for(spike_labels))
	SpikeLabels const& get_spike_labels() const SYMBOL_VISIBLE;

private:
	std::shared_ptr<Network> m_network;
	Graph m_graph;
	std::optional<Graph::vertex_descriptor> m_event_input_vertex;
	std::optional<Graph::vertex_descriptor> m_event_output_vertex;
	std::optional<Graph::vertex_descriptor> m_madc_sample_output_vertex;
	SpikeLabels m_spike_labels;

	friend NetworkGraph build_network_graph(
	    std::shared_ptr<Network> const& network, RoutingResult const& routing_result);
};

} // namespace network

} // namespace grenade::vx
