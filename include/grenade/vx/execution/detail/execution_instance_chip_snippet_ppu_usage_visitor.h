#pragma once
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/ppu/neuron_view_handle.h"
#include "grenade/vx/ppu/synapse_array_view_handle.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"
#include "stadls/vx/v3/playback_program_builder.h"
#include <optional>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <log4cxx/logger.h>

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
		    grenade::common::VertexOnTopology,
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
	 * @param topology Topology to use
	 * @param execution_instance_vertex_descriptor Vertex descriptor of execution instance to visit
	 * @param input_data Input data to visit
	 * @param chip_on_connection Chip on connection to visit for
	 * @param snippet_index Index of realtime snippet
	 */
	ExecutionInstanceChipSnippetPPUUsageVisitor(
	    grenade::common::LinkedTopology const& topology,
	    grenade::common::VertexOnTopology const& execution_instance_vertex_descriptor,
	    grenade::common::InputData const& input_data,
	    common::ChipOnConnection const& chip_on_connection,
	    size_t snippet_index) SYMBOL_VISIBLE;

	/**
	 * Perform visit operation.
	 * @return Result
	 */
	Result operator()() const SYMBOL_VISIBLE;

private:
	grenade::common::LinkedTopology const& m_topology;
	grenade::common::VertexOnTopology m_execution_instance_vertex_descriptor;
	grenade::common::InputData const& m_input_data;
	common::ChipOnConnection m_chip_on_connection;
	size_t m_snippet_index;
	log4cxx::LoggerPtr m_logger;

	/**
	 * Process single vertex.
	 * This function is called in preprocess.
	 * @param vertex Vertex descriptor
	 * @param data Data associated with vertex
	 * @param result Result to process into
	 */
	template <typename Vertex>
	void process(
	    grenade::common::VertexOnTopology const vertex, Vertex const& data, Result& result) const;

	std::unordered_map<
	    std::type_index,
	    std::function<void(
	        grenade::common::VertexOnTopology const&, grenade::common::Vertex const&, Result&)>>
	    m_visitor;

	template <typename Vertex>
	void register_process();
};

} // namespace grenade::vx::execution::detail
