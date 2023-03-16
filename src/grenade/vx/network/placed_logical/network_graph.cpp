#include "grenade/vx/network/placed_logical/network_graph.h"

namespace grenade::vx::network::placed_logical {

std::shared_ptr<Network> const& NetworkGraph::get_network() const
{
	return m_network;
}

std::shared_ptr<network::placed_atomic::Network> const& NetworkGraph::get_hardware_network() const
{
	return m_hardware_network;
}

NetworkGraph::PopulationTranslation const& NetworkGraph::get_population_translation() const
{
	return m_population_translation;
}

NetworkGraph::NeuronTranslation const& NetworkGraph::get_neuron_translation() const
{
	return m_neuron_translation;
}

NetworkGraph::ProjectionTranslation const& NetworkGraph::get_projection_translation() const
{
	return m_projection_translation;
}

NetworkGraph::PlasticityRuleTranslation const& NetworkGraph::get_plasticity_rule_translation() const
{
	return m_plasticity_rule_translation;
}

} // namespace grenade::vx::network::placed_logical
