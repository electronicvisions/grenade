#pragma once
#include "grenade/vx/ppu/synapse_array_view_handle.h"
#include "grenade/vx/vertex/plasticity_rule.h"
#include "halco/hicann-dls/vx/v2/synapse.h"
#include "hate/visibility.h"
#include <tuple>
#include <vector>

namespace grenade::vx {

class PPUProgramGenerator
{
public:
	PPUProgramGenerator() = default;

	void add(
	    vertex::PlasticityRule const& rule,
	    std::vector<
	        std::pair<halco::hicann_dls::vx::v2::SynramOnDLS, ppu::SynapseArrayViewHandle>> const&
	        synapses) SYMBOL_VISIBLE;

	std::vector<std::string> done() SYMBOL_VISIBLE;

private:
	std::vector<std::tuple<
	    vertex::PlasticityRule,
	    std::vector<
	        std::pair<halco::hicann_dls::vx::v2::SynramOnDLS, ppu::SynapseArrayViewHandle>>>>
	    m_plasticity_rules;
};

} // namespace grenade::vx
