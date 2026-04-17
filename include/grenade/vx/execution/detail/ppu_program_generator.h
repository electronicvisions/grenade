#pragma once
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/ppu/neuron_view_handle.h"
#include "grenade/vx/ppu/synapse_array_view_handle.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "halco/hicann-dls/vx/v3/synram.h"
#include "hate/visibility.h"
#include <tuple>
#include <vector>

namespace grenade::vx::execution::detail {

class PPUProgramGenerator
{
public:
	PPUProgramGenerator() = default;

	void add(
	    grenade::common::VertexOnTopology descriptor,
	    signal_flow::vertex::PlasticityRule const& rule,
	    signal_flow::vertex::PlasticityRule::Parameterization const& rule_parameterization,
	    signal_flow::vertex::PlasticityRule::Dynamics const& rule_dynamics,
	    std::vector<
	        std::pair<halco::hicann_dls::vx::v3::SynramOnDLS, ppu::SynapseArrayViewHandle>> const&
	        synapses,
	    std::vector<
	        std::pair<halco::hicann_dls::vx::v3::NeuronRowOnDLS, ppu::NeuronViewHandle>> const&
	        neurons,
	    size_t snippet_index) SYMBOL_VISIBLE;

	std::vector<std::string> done() SYMBOL_VISIBLE;

	bool has_periodic_cadc_readout = false;
	bool has_periodic_cadc_readout_on_dram = false;
	size_t num_periodic_cadc_samples = 0;

private:
	std::vector<std::tuple<
	    grenade::common::VertexOnTopology,
	    signal_flow::vertex::PlasticityRule,
	    signal_flow::vertex::PlasticityRule::Parameterization,
	    signal_flow::vertex::PlasticityRule::Dynamics,
	    std::vector<std::pair<halco::hicann_dls::vx::v3::SynramOnDLS, ppu::SynapseArrayViewHandle>>,
	    std::vector<std::pair<halco::hicann_dls::vx::v3::NeuronRowOnDLS, ppu::NeuronViewHandle>>,
	    size_t>>
	    m_plasticity_rules;
};

} // namespace grenade::vx::execution::detail
