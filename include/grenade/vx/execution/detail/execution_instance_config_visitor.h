#pragma once
#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/ppu/neuron_view_handle.h"
#include "grenade/vx/ppu/synapse_array_view_handle.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"
#include "stadls/vx/v3/playback_program_builder.h"
#include <optional>
#include <tuple>
#include <vector>

namespace grenade::vx::execution::detail {

/**
 * Visitor of graph vertices of a single execution instance for construction of the initial
 * configuration.
 * The result is applied to a given configuration object.
 */
class ExecutionInstanceConfigVisitor
{
public:
	struct PpuUsage
	{
		bool has_periodic_cadc_readout = false;
		bool has_periodic_cadc_readout_on_dram = false;
		bool has_cadc_readout = false;
		std::vector<std::tuple<
		    signal_flow::Graph::vertex_descriptor,
		    signal_flow::vertex::PlasticityRule,
		    std::vector<
		        std::pair<halco::hicann_dls::vx::v3::SynramOnDLS, ppu::SynapseArrayViewHandle>>,
		    std::vector<
		        std::pair<halco::hicann_dls::vx::v3::NeuronRowOnDLS, ppu::NeuronViewHandle>>,
		    size_t>>
		    plasticity_rules;

		PpuUsage() = default;
		PpuUsage(PpuUsage const& other) = default;
		PpuUsage(PpuUsage&& other);
		PpuUsage& operator=(PpuUsage&& other);
		PpuUsage& operator=(PpuUsage& other) = default;

		/**
		 * Order has_periodic_cadc_readout and concatenate plasticity_rules
		 */
		PpuUsage& operator+=(PpuUsage&& other);
	};

	/**
	 * Construct visitor.
	 * @param graph Graph to use for locality and property lookup
	 * @param execution_instance Local execution instance to visit
	 * @param config Configuration to alter
	 */
	ExecutionInstanceConfigVisitor(
	    signal_flow::Graph const& graph,
	    common::ExecutionInstanceID const& execution_instance,
	    lola::vx::v3::Chip& config,
	    size_t realtime_column_index) SYMBOL_VISIBLE;

	/**
	 * Perform visit operation and generate initial configuration.
	 * @return PpuUsage
	 */
	std::tuple<
	    PpuUsage,
	    halco::common::typed_array<bool, halco::hicann_dls::vx::v3::NeuronResetOnDLS>>
	operator()() SYMBOL_VISIBLE;

private:
	signal_flow::Graph const& m_graph;
	common::ExecutionInstanceID m_execution_instance;

	lola::vx::v3::Chip& m_config;

	size_t m_realtime_column_index;

	halco::common::typed_array<bool, halco::hicann_dls::vx::v3::NeuronResetOnDLS>
	    m_enabled_neuron_resets;
	bool m_has_periodic_cadc_readout;
	bool m_has_periodic_cadc_readout_on_dram;
	bool m_has_cadc_readout;
	bool m_used_madc;

	std::vector<std::tuple<
	    signal_flow::Graph::vertex_descriptor,
	    signal_flow::vertex::PlasticityRule,
	    std::vector<std::pair<halco::hicann_dls::vx::v3::SynramOnDLS, ppu::SynapseArrayViewHandle>>,
	    std::vector<std::pair<halco::hicann_dls::vx::v3::NeuronRowOnDLS, ppu::NeuronViewHandle>>,
	    size_t>>
	    m_plasticity_rules;

	/**
	 * Process single vertex.
	 * This function is called in preprocess.
	 * @param vertex Vertex descriptor
	 * @param data Data associated with vertex
	 */
	template <typename Vertex>
	void process(signal_flow::Graph::vertex_descriptor const vertex, Vertex const& data);

	/**
	 * Preprocess by single visit of all local vertices.
	 */
	void pre_process() SYMBOL_VISIBLE;
};

} // namespace grenade::vx::execution::detail
