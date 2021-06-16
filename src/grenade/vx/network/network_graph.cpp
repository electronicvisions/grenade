#include "grenade/vx/network/network_graph.h"

namespace grenade::vx::network {

std::shared_ptr<Network> const& NetworkGraph::get_network() const
{
	return m_network;
}

Graph const& NetworkGraph::get_graph() const
{
	return m_graph;
}

std::optional<Graph::vertex_descriptor> NetworkGraph::get_event_input_vertex() const
{
	return m_event_input_vertex;
}

std::optional<Graph::vertex_descriptor> NetworkGraph::get_event_output_vertex() const
{
	return m_event_output_vertex;
}

std::optional<Graph::vertex_descriptor> NetworkGraph::get_madc_sample_output_vertex() const
{
	return m_madc_sample_output_vertex;
}

NetworkGraph::SpikeLabels const& NetworkGraph::get_spike_labels() const
{
	return m_spike_labels;
}

} // namespace grenade::vx::network
