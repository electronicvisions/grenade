#pragma once
#include <atomic>
#include <optional>
#include <set>
#include <vector>

#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/input_data.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/detail/execution_instance_node.h"
#include "grenade/vx/execution/detail/execution_instance_snippet_data.h"
#include "grenade/vx/execution/detail/generator/neuron_reset_mask.h"
#include "grenade/vx/execution/detail/generator/ppu.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"
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

namespace grenade::vx::execution::detail {

/**
 * Builder for a single ExecutionInstance.
 * Vertices are processed resulting in a playback sequence and result structure.
 * Once executed, the result structure stores resulting measurements which can be processed to
 * be fed back into a graph executor.
 */
class ExecutionInstanceChipSnippetRealtimeExecutor
{
public:
	/**
	 * Construct builder.
	 * @param topology Topology to use
	 * @param execution_instance_vertex_descriptor Vertex descriptor of execution instance to visit
	 * @param input_data Input data to visit
	 * @param data Data accessor
	 * @param post_vertices Storage for collecting vertices to visit again after hardware execution
	 * @param ppu_symbols PPU symbols to use for PPU control
	 * @param timed_recording_index_offset Index offset of each plasticity rule for this snippet,
	 * which the builder generates
	 */
	ExecutionInstanceChipSnippetRealtimeExecutor(
	    grenade::common::LinkedTopology const& topology,
	    grenade::common::VertexOnTopology const& execution_instance_vertex_descriptor,
	    grenade::common::InputData const& input_data,
	    ExecutionInstanceSnippetData& data,
	    std::vector<grenade::common::VertexOnTopology>& post_vertices,
	    std::optional<lola::vx::v3::PPUElfFile::symbols_type> const& ppu_symbols,
	    std::map<signal_flow::vertex::PlasticityRule::ID, size_t> const&
	        timed_recording_index_offset) SYMBOL_VISIBLE;

	struct Usages
	{
		bool madc_recording;
		bool event_recording;
		halco::common::typed_array<
		    std::set<signal_flow::vertex::CADCMembraneReadoutView::Mode>,
		    halco::hicann_dls::vx::v3::HemisphereOnDLS>
		    cadc_recording;
	};

	/**
	 * Preprocess by single visit of all local vertices.
	 */
	Usages pre_process() SYMBOL_VISIBLE;

	struct RealtimeSnippet
	{
		stadls::vx::v3::AbsoluteTimePlaybackProgramBuilder builder;
		std::vector<stadls::vx::v3::PlaybackProgramBuilder> ppu_finish_builder;
		haldls::vx::v3::Timer::Value pre_realtime_duration;
		haldls::vx::v3::Timer::Value realtime_duration;
	};

	struct Program
	{
		std::vector<RealtimeSnippet> realtimes;
	};

	/**
	 * Generate playback sequence.
	 * @return PlaybackPrograms generated via local graph traversal
	 */
	Program generate(Usages before, Usages after) SYMBOL_VISIBLE;

	struct PostProcessable
	{
		std::vector<stadls::vx::v3::PlaybackProgram> realtime;
		std::vector<halco::common::typed_array<
		    std::vector<stadls::vx::v3::ContainerTicket>,
		    halco::hicann_dls::vx::PPUOnDLS>>
		    cadc_readout_tickets;
		std::optional<ExecutionInstanceNode::PeriodicCADCReadoutTimes> periodic_cadc_readout_times;
	};

	/**
	 * Postprocess by visit of all local vertices to be post processed after execution.
	 * This resets the internal state of the builder to be ready for the next time step.
	 * @return signal_flow::OutputData of locally computed results
	 */
	std::chrono::nanoseconds post_process(PostProcessable&& post_processable) SYMBOL_VISIBLE;

	/**
	 * Switch to enable CADC baseline read before each sent input vector.
	 * If disabled, the membrane resting potential is assumed to reside at CADC value 128.
	 */
	bool enable_cadc_baseline = true;

	static haldls::vx::v3::Timer::Value const wait_before_realtime;
	static haldls::vx::v3::Timer::Value const wait_after_realtime;

	/**
	 * Process single vertex.
	 * This function is called in both preprocess and postprocess depending on whether the vertex
	 * requires post-execution processing.
	 * @param vertex Vertex descriptor
	 * @param data Data associated with vertex
	 */
	template <typename Vertex>
	void process(grenade::common::VertexOnTopology const vertex, Vertex const& data);

private:
	grenade::common::LinkedTopology const& m_topology;
	grenade::common::VertexOnTopology m_execution_instance_vertex_descriptor;
	common::ChipOnConnection m_chip_on_connection;

	grenade::common::InputData const& m_input_data;
	ExecutionInstanceSnippetData& m_data;

	std::optional<lola::vx::v3::PPUElfFile::symbols_type> m_ppu_symbols;

	std::vector<grenade::common::VertexOnTopology>& m_post_vertices;

	std::vector<stadls::vx::v3::PlaybackProgram> m_chunked_program;

	std::vector<grenade::vx::signal_flow::TimedSpikeToChipSequence> m_event_input;
	std::optional<grenade::common::VertexOnTopology> m_event_output_vertex;

	bool m_postprocessing;

	typedef halco::common::typed_array<bool, halco::hicann_dls::vx::v3::HemisphereOnDLS>
	    ticket_request_type;
	ticket_request_type m_ticket_requests;

	ExecutionInstanceNode::PeriodicCADCReadoutTimes m_periodic_cadc_readout_times;

	struct BatchEntry
	{
		typedef halco::common::typed_array<
		    std::optional<stadls::vx::v3::ContainerTicket>,
		    halco::hicann_dls::vx::PPUOnDLS>
		    ticket_ppu_type;

		halco::common::typed_array<
		    std::optional<stadls::vx::AbsoluteTimePlaybackProgramContainerTicket>,
		    halco::hicann_dls::vx::PPUOnDLS>
		    m_ppu_result;

		typedef std::optional<stadls::vx::AbsoluteTimePlaybackProgramContainerTicket>
		    event_guard_ticket_type;
		event_guard_ticket_type m_ticket_events_begin;
		event_guard_ticket_type m_ticket_events_end;

		typedef halco::common::typed_array<
		    std::vector<stadls::vx::v3::ContainerTicket>,
		    halco::hicann_dls::vx::PPUOnDLS>
		    ticket_extmem_type;

		ticket_extmem_type m_extmem_result;

		ticket_ppu_type m_ppu_scheduler_event_drop_count;
		std::vector<ticket_ppu_type> m_ppu_timer_event_drop_count;
		ticket_ppu_type m_ppu_scheduler_finished;
		ticket_ppu_type m_ppu_mailbox;
		std::map<grenade::common::VertexOnTopology, std::vector<stadls::vx::v3::ContainerTicket>>
		    m_plasticity_rule_recorded_scratchpad_memory;

		std::vector<generator::PPUCommand::Result> ppu_command_results;
	};

	std::vector<BatchEntry> m_batch_entries;

	generator::NeuronResetMask m_neuron_resets;
	// Optional vertex descriptor of MADC readout if the execution instance contains such
	std::optional<grenade::common::VertexOnTopology> m_madc_readout_vertex;

	std::optional<signal_flow::vertex::CADCMembraneReadoutView::Mode> m_cadc_readout_mode;

	bool m_has_plasticity_rule{false};

	std::map<signal_flow::vertex::PlasticityRule::ID, size_t> m_timed_recording_index_offset;

	/**
	 * Filter events via batch entry runtime and recording interval.
	 * The input data is to be modified because of sorting in-place.
	 * @param filtered_data Filtered data per batch
	 * @param data Event sequence
	 */
	template <typename T>
	void filter_events(std::vector<std::vector<T>>& filtered_data, std::vector<T>&& data) const;
};

} // namespace grenade::vx::execution::detail

#include "grenade/vx/execution/detail/execution_instance_chip_snippet_realtime_executor.tcc"
