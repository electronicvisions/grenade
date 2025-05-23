#pragma once
#include "grenade/common/execution_instance_id.h"
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
 * Visitor of graph vertices of a single execution instance for extraction of used PPU resources.
 */
class ExecutionInstanceChipSnippetPPUUsageVisitor
{
public:
	struct Result
	{
		bool has_periodic_cadc_readout;
		bool has_periodic_cadc_readout_on_dram;
		bool has_cadc_readout;
		std::vector<std::tuple<
		    signal_flow::Graph::vertex_descriptor,
		    signal_flow::vertex::PlasticityRule,
		    std::vector<
		        std::pair<halco::hicann_dls::vx::v3::SynramOnDLS, ppu::SynapseArrayViewHandle>>,
		    std::vector<
		        std::pair<halco::hicann_dls::vx::v3::NeuronRowOnDLS, ppu::NeuronViewHandle>>,
		    size_t>>
		    plasticity_rules;
		halco::common::typed_array<bool, halco::hicann_dls::vx::v3::NeuronResetOnDLS>
		    enabled_neuron_resets;

		Result() SYMBOL_VISIBLE;
		Result(Result const& other) = default;
		Result(Result&& other);
		Result& operator=(Result&& other);
		Result& operator=(Result& other) = default;

		/**
		 * Order has_periodic_cadc_readout and concatenate plasticity_rules, or
		 * enabled_neuron_resets.
		 */
		Result& operator+=(Result&& other);
	};

	/**
	 * Construct visitor.
	 * @param graph Graph to use for locality and property lookup
	 * @param execution_instance Local execution instance to visit
	 * @param snippet_index Index of realtime snippet
	 */
	ExecutionInstanceChipSnippetPPUUsageVisitor(
	    signal_flow::Graph const& graph,
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::ExecutionInstanceID const& execution_instance,
	    size_t snippet_index) SYMBOL_VISIBLE;

	/**
	 * Perform visit operation.
	 * @return Result
	 */
	Result operator()() const SYMBOL_VISIBLE;

private:
	signal_flow::Graph const& m_graph;
	common::ChipOnConnection m_chip_on_connection;
	grenade::common::ExecutionInstanceID m_execution_instance;
	size_t m_snippet_index;

	/**
	 * Process single vertex.
	 * This function is called in preprocess.
	 * @param vertex Vertex descriptor
	 * @param data Data associated with vertex
	 * @param result Result to process into
	 */
	template <typename Vertex>
	void process(
	    signal_flow::Graph::vertex_descriptor const vertex,
	    Vertex const& data,
	    Result& result) const;
};

} // namespace grenade::vx::execution::detail
