#pragma once
#include "grenade/vx/ppu/neuron_view_handle.h"
#include "grenade/vx/ppu/synapse_array_view_handle.h"
#include "grenade/vx/signal_flow/graph.h"
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
	    signal_flow::Graph::vertex_descriptor descriptor,
	    signal_flow::vertex::PlasticityRule const& rule,
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
	    signal_flow::Graph::vertex_descriptor,
	    signal_flow::vertex::PlasticityRule,
	    std::vector<std::pair<halco::hicann_dls::vx::v3::SynramOnDLS, ppu::SynapseArrayViewHandle>>,
	    std::vector<std::pair<halco::hicann_dls::vx::v3::NeuronRowOnDLS, ppu::NeuronViewHandle>>,
	    size_t>>
	    m_plasticity_rules;
};

} // namespace grenade::vx::execution::detail
