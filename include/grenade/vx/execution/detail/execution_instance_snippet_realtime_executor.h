#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/execution/detail/execution_instance_chip_snippet_realtime_executor.h"
#include "grenade/vx/execution/detail/execution_instance_node.h"
#include "grenade/vx/execution/detail/execution_instance_snippet_data.h"
#include "grenade/vx/execution/detail/generator/neuron_reset_mask.h"
#include "grenade/vx/execution/detail/generator/ppu.h"
#include "grenade/vx/signal_flow/data_snippet.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input_data_snippet.h"
#include "grenade/vx/signal_flow/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/ppu.h"
#include "haldls/vx/v3/synapse_driver.h"
#include "hate/visibility.h"
#include "lola/vx/v3/cadc.h"
#include "lola/vx/v3/ppu.h"
#include "lola/vx/v3/synapse.h"
#include "stadls/vx/absolute_time_playback_program_container_ticket.h"
#include "stadls/vx/v3/container_ticket.h"
#include "stadls/vx/v3/playback_generator.h"
#include "stadls/vx/v3/playback_program.h"
#include "stadls/vx/v3/playback_program_builder.h"
#include <atomic>
#include <chrono>
#include <optional>
#include <set>
#include <vector>

namespace grenade::vx::execution::detail {

/**
 * Builder for a single ExecutionInstance.
 * Vertices are processed resulting in a playback sequence and result structure.
 * Once executed, the result structure stores resulting measurements which can be processed to
 * be fed back into a graph executor.
 */
class ExecutionInstanceSnippetRealtimeExecutor
{
public:
	/**
	 * Construct builder.
	 * @param graph Graph to use for locality and property lookup
	 * @param execution_instance Local execution instance to build for
	 * @param input_list Input list to use for input data lookup
	 * @param data_output Data output from depended-on executions to use for data lookup
	 * @param chip_config Chip configuration to use
	 * @param realtime_column_index Index of realtime column this builder generates
	 * @param timed_recording_index_offset Index offset of each plasticity rule for this snippet,
	 * which the builder generates
	 */
	ExecutionInstanceSnippetRealtimeExecutor(
	    signal_flow::Graph const& graph,
	    grenade::common::ExecutionInstanceID const& execution_instance,
	    signal_flow::InputDataSnippet const& input_list,
	    signal_flow::DataSnippet const& data_output,
	    std::optional<lola::vx::v3::PPUElfFile::symbols_type> const& ppu_symbols,
	    size_t realtime_column_index,
	    std::map<signal_flow::vertex::PlasticityRule::ID, size_t> const&
	        timed_recording_index_offset) SYMBOL_VISIBLE;

	typedef ExecutionInstanceChipSnippetRealtimeExecutor::Usages Usages;

	/**
	 * Preprocess by single visit of all local vertices.
	 */
	Usages pre_process() SYMBOL_VISIBLE;

	typedef ExecutionInstanceChipSnippetRealtimeExecutor::Ret Ret;

	/**
	 * Generate playback sequence.
	 * @return PlaybackPrograms generated via local graph traversal
	 */
	Ret generate(Usages before, Usages after) SYMBOL_VISIBLE;

	struct Result
	{
		signal_flow::DataSnippet data;

		std::chrono::nanoseconds total_realtime_duration;
	};

	typedef ExecutionInstanceChipSnippetRealtimeExecutor::PostProcessable PostProcessable;

	/**
	 * Postprocess by visit of all local vertices to be post processed after execution.
	 * This resets the internal state of the builder to be ready for the next time step.
	 */
	Result post_process(PostProcessable const& post_processable) SYMBOL_VISIBLE;

private:
	signal_flow::Graph const& m_graph;
	grenade::common::ExecutionInstanceID m_execution_instance;

	std::unique_ptr<ExecutionInstanceSnippetData> m_data;

	std::unique_ptr<std::vector<signal_flow::Graph::vertex_descriptor>> m_post_vertices;

	ExecutionInstanceChipSnippetRealtimeExecutor m_chip_executor;

	/**
	 * Check if any incoming vertex requires post processing.
	 * @param descriptor Vertex descriptor to check for
	 * @return Boolean value
	 */
	bool inputs_available(signal_flow::Graph::vertex_descriptor const descriptor) const
	    SYMBOL_VISIBLE;

	/**
	 * Process single vertex.
	 * This function is called in both preprocess and postprocess depending on whether the vertex
	 * requires post-execution processing.
	 * @param vertex Vertex descriptor
	 * @param data Data associated with vertex
	 */
	template <typename Vertex>
	void process(signal_flow::Graph::vertex_descriptor const vertex, Vertex const& data);

	/**
	 * Get whether input data is complete for the local execution instance.
	 * @param input_data Input data to check
	 * @return Boolean value
	 */
	bool input_data_matches_graph(signal_flow::InputDataSnippet const& input_data) const;
};

} // namespace grenade::vx::execution::detail
