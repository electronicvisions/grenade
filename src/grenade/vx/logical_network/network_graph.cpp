#include "grenade/vx/logical_network/network_graph.h"

namespace grenade::vx::logical_network {

std::shared_ptr<Network> const& NetworkGraph::get_network() const
{
	return m_network;
}

std::shared_ptr<network::Network> const& NetworkGraph::get_hardware_network() const
{
	return m_hardware_network;
}

NetworkGraph::PopulationTranslation const& NetworkGraph::get_population_translation() const
{
	return m_population_translation;
}

NetworkGraph::ProjectionTranslation const& NetworkGraph::get_projection_translation() const
{
	return m_projection_translation;
}

} // namespace grenade::vx::logical_network
