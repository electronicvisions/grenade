#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_atomic/network.h"
#include "grenade/vx/network/placed_logical/network.h"
#include "grenade/vx/network/placed_logical/population.h"
#include "hate/visibility.h"
#include <map>
#include <optional>

namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

/**
 * Logical network representation.
 */
struct GENPYBIND(visible) NetworkGraph
{
	NetworkGraph() = default;

	/** Underlying network. */
	GENPYBIND(getter_for(network))
	std::shared_ptr<Network> const& get_network() const SYMBOL_VISIBLE;

	/** Hardware network. */
	GENPYBIND(getter_for(hardware_network))
	std::shared_ptr<network::placed_atomic::Network> const& get_hardware_network() const
	    SYMBOL_VISIBLE;

	/** Translation between logical and hardware populations. */
	typedef std::map<PopulationDescriptor, network::placed_atomic::PopulationDescriptor>
	    PopulationTranslation;
	GENPYBIND(getter_for(population_translation))
	PopulationTranslation const& get_population_translation() const SYMBOL_VISIBLE;

	/** Translation between logical and hardware neurons in populations. */
	typedef std::map<
	    PopulationDescriptor,
	    std::vector<
	        std::map<halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron, std::vector<size_t>>>>
	    NeuronTranslation;
	GENPYBIND(getter_for(neuron_translation))
	NeuronTranslation const& get_neuron_translation() const SYMBOL_VISIBLE;

	/** Translation between logical and hardware projections. */
	typedef std::multimap<
	    std::pair<ProjectionDescriptor, size_t>,
	    std::pair<network::placed_atomic::ProjectionDescriptor, size_t>>
	    ProjectionTranslation;
	GENPYBIND(getter_for(projection_translation))
	ProjectionTranslation const& get_projection_translation() const SYMBOL_VISIBLE;

	/** Translation between logical and hardware populations. */
	typedef std::map<PlasticityRuleDescriptor, network::placed_atomic::PlasticityRuleDescriptor>
	    PlasticityRuleTranslation;
	GENPYBIND(getter_for(plasticity_rule_translation))
	PlasticityRuleTranslation const& get_plasticity_rule_translation() const SYMBOL_VISIBLE;

private:
	std::shared_ptr<Network> m_network;
	std::shared_ptr<network::placed_atomic::Network> m_hardware_network;
	PopulationTranslation m_population_translation;
	NeuronTranslation m_neuron_translation;
	ProjectionTranslation m_projection_translation;
	PlasticityRuleTranslation m_plasticity_rule_translation;

	friend NetworkGraph build_network_graph(std::shared_ptr<Network> const& network);
};

} // namespace grenade::vx::network::placed_logical
