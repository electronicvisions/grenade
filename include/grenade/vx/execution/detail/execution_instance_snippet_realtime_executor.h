#pragma once
#include "grenade/common/input_data.h"
#include "grenade/common/output_data.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/detail/execution_instance_chip_snippet_realtime_executor.h"
#include "grenade/vx/execution/detail/execution_instance_node.h"
#include "grenade/vx/execution/detail/execution_instance_snippet_data.h"
#include "grenade/vx/execution/detail/generator/neuron_reset_mask.h"
#include "grenade/vx/execution/detail/generator/ppu.h"
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
	 * @param topology Topology to use
	 * @param execution_instance_vertex_descriptor Vertex descriptor of execution instance to visit
	 * @param chips_on_connection Chip identifiers on connection to use
	 * @param input_data Input data to visit
	 * @param output_data Data output from depended-on executions to use for data lookup
	 * @param ppu_symbols PPU symbols to use
	 * @param timed_recording_index_offset Index offset of each plasticity rule for this snippet,
	 * which the builder generates
	 */
	ExecutionInstanceSnippetRealtimeExecutor(
	    grenade::common::LinkedTopology const& topology,
	    grenade::common::VertexOnTopology const& execution_instance_vertex_descriptor,
	    std::vector<common::ChipOnConnection> const& chips_on_connection,
	    grenade::common::InputData const& input_data,
	    grenade::common::OutputData const& output_data,
	    std::map<
	        common::ChipOnConnection,
	        std::reference_wrapper<
	            std::optional<lola::vx::v3::PPUElfFile::symbols_type> const>> const& ppu_symbols,
	    std::map<
	        common::ChipOnConnection,
	        std::map<signal_flow::vertex::PlasticityRule::ID, size_t>> const&
	        timed_recording_index_offset) SYMBOL_VISIBLE;

	typedef std::map<common::ChipOnConnection, ExecutionInstanceChipSnippetRealtimeExecutor::Usages>
	    Usages;

	/**
	 * Preprocess by single visit of all local vertices.
	 */
	Usages pre_process() SYMBOL_VISIBLE;

	typedef std::
	    map<common::ChipOnConnection, ExecutionInstanceChipSnippetRealtimeExecutor::Program>
	        Program;

	/**
	 * Generate playback sequence.
	 * @return PlaybackPrograms generated via local graph traversal
	 */
	Program generate(Usages before, Usages after) SYMBOL_VISIBLE;

	struct Result
	{
		grenade::common::OutputData data;

		std::map<common::ChipOnConnection, std::chrono::nanoseconds> total_realtime_duration;
	};

	typedef std::
	    map<common::ChipOnConnection, ExecutionInstanceChipSnippetRealtimeExecutor::PostProcessable>
	        PostProcessable;

	/**
	 * Postprocess by visit of all local vertices to be post processed after execution.
	 * This resets the internal state of the builder to be ready for the next time step.
	 */
	Result post_process(PostProcessable&& post_processable) SYMBOL_VISIBLE;

private:
	grenade::common::LinkedTopology const& m_topology;
	grenade::common::VertexOnTopology m_execution_instance_vertex_descriptor;

	grenade::common::InputData const& m_input_data;
	std::unique_ptr<ExecutionInstanceSnippetData> m_data;

	std::unique_ptr<std::vector<grenade::common::VertexOnTopology>> m_post_vertices;

	std::map<common::ChipOnConnection, ExecutionInstanceChipSnippetRealtimeExecutor>
	    m_chip_executors;

	log4cxx::LoggerPtr m_logger;

	/**
	 * Check if any incoming vertex requires post processing.
	 * @param descriptor Vertex descriptor to check for
	 * @return Boolean value
	 */
	bool inputs_available(grenade::common::VertexOnTopology const descriptor) const SYMBOL_VISIBLE;

	/**
	 * Process single vertex.
	 * This function is called in both preprocess and postprocess depending on whether the vertex
	 * requires post-execution processing.
	 * @param vertex Vertex descriptor
	 * @param data Data associated with vertex
	 */
	template <typename Vertex>
	void process(grenade::common::VertexOnTopology const vertex, Vertex const& data);

	std::unordered_map<
	    std::type_index,
	    std::function<void(
	        grenade::common::VertexOnTopology const&, grenade::common::Vertex const&)>>
	    m_visitor;

	template <typename Vertex>
	void register_process();
};

} // namespace grenade::vx::execution::detail
