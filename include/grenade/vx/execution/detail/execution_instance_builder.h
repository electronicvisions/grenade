#pragma once
#include <atomic>
#include <optional>
#include <set>
#include <vector>

#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/execution/detail/generator/neuron_reset_mask.h"
#include "grenade/vx/signal_flow/execution_instance_playback_hooks.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/io_data_map.h"
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

namespace grenade::vx::execution::detail {

/**
 * Builder for a single ExecutionInstance.
 * Vertices are processed resulting in a playback sequence and result structure.
 * Once executed, the result structure stores resulting measurements which can be processed to
 * be fed back into a graph executor.
 */
class ExecutionInstanceBuilder
{
public:
	/**
	 * Construct builder.
	 * @param graph Graph to use for locality and property lookup
	 * @param execution_instance Local execution instance to build for
	 * @param input_list Input list to use for input data lookup
	 * @param data_output Data output from depended-on executions to use for data lookup
	 * @param chip_config Chip configuration to use
	 * @param playback_hooks Playback sequences to inject
	 */
	ExecutionInstanceBuilder(
	    signal_flow::Graph const& graph,
	    common::ExecutionInstanceID const& execution_instance,
	    signal_flow::IODataMap const& input_list,
	    signal_flow::IODataMap const& data_output,
	    std::optional<lola::vx::v3::PPUElfFile::symbols_type> const& ppu_symbols,
	    signal_flow::ExecutionInstancePlaybackHooks& playback_hooks) SYMBOL_VISIBLE;

	struct Usages
	{
		bool madc_recording;
		bool event_recording;
	};

	/**
	 * Preprocess by single visit of all local vertices.
	 */
	Usages pre_process() SYMBOL_VISIBLE;

	struct RealtimeSnippet
	{
		stadls::vx::v3::AbsoluteTimePlaybackProgramBuilder builder;
		stadls::vx::v3::PlaybackProgramBuilder ppu_finish_builder;
		haldls::vx::v3::Timer::Value pre_realtime_duration;
		haldls::vx::v3::Timer::Value realtime_duration;
	};

	struct Ret
	{
		stadls::vx::v3::PlaybackProgramBuilder start_ppu;
		stadls::vx::v3::PlaybackProgramBuilder arm_madc;
		std::vector<RealtimeSnippet> realtimes;
		stadls::vx::v3::PlaybackProgramBuilder stop_ppu;
	};

	/**
	 * Generate playback sequence.
	 * @return PlaybackPrograms generated via local graph traversal
	 */
	Ret generate(Usages before, Usages after) SYMBOL_VISIBLE;

	/**
	 * Postprocess by visit of all local vertices to be post processed after execution.
	 * This resets the internal state of the builder to be ready for the next time step.
	 * @return signal_flow::IODataMap of locally computed results
	 */
	signal_flow::IODataMap post_process(
	    std::vector<stadls::vx::v3::PlaybackProgram> const& realtime) SYMBOL_VISIBLE;
	void post_process(signal_flow::Graph::vertex_descriptor const vertex) SYMBOL_VISIBLE;

	/**
	 * Switch to enable CADC baseline read before each sent input vector.
	 * If disabled, the membrane resting potential is assumed to reside at CADC value 128.
	 */
	bool enable_cadc_baseline = true;

private:
	signal_flow::Graph const& m_graph;
	common::ExecutionInstanceID m_execution_instance;
	signal_flow::IODataMap const& m_input_list;
	signal_flow::IODataMap const& m_data_output;

	std::optional<lola::vx::v3::PPUElfFile::symbols_type> m_ppu_symbols;

	signal_flow::ExecutionInstancePlaybackHooks& m_playback_hooks;

	std::vector<signal_flow::Graph::vertex_descriptor> m_post_vertices;

	std::vector<stadls::vx::v3::PlaybackProgram> m_chunked_program;

	std::optional<signal_flow::Graph::vertex_descriptor> m_event_input_vertex;
	std::optional<signal_flow::Graph::vertex_descriptor> m_event_output_vertex;

	bool m_postprocessing;

	signal_flow::IODataMap m_local_data;
	signal_flow::IODataMap m_local_data_output;

	typedef halco::common::typed_array<bool, halco::hicann_dls::vx::v3::HemisphereOnDLS>
	    ticket_request_type;
	ticket_request_type m_ticket_requests;

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
		    std::optional<stadls::vx::v3::ContainerTicket>,
		    halco::hicann_dls::vx::PPUOnDLS>
		    ticket_extmem_type;

		ticket_extmem_type m_extmem_result;

		ticket_ppu_type m_ppu_scheduler_event_drop_count;
		std::vector<ticket_ppu_type> m_ppu_timer_event_drop_count;
		ticket_ppu_type m_ppu_scheduler_finished;
		ticket_ppu_type m_ppu_mailbox;
		std::map<signal_flow::Graph::vertex_descriptor, ticket_extmem_type>
		    m_plasticity_rule_recorded_scratchpad_memory;

		typedef std::map<
		    std::string,
		    std::variant<
		        std::map<
		            halco::hicann_dls::vx::v3::HemisphereOnDLS,
		            stadls::vx::v3::ContainerTicket>,
		        stadls::vx::v3::ContainerTicket>>
		    ppu_symbol_ticket_type;
		ppu_symbol_ticket_type ppu_symbols;
	};

	std::vector<BatchEntry> m_batch_entries;

	generator::NeuronResetMask m_neuron_resets;
	// Optional vertex descriptor of MADC readout if the execution instance contains such
	std::optional<signal_flow::Graph::vertex_descriptor> m_madc_readout_vertex;

	std::optional<signal_flow::vertex::CADCMembraneReadoutView::Mode> m_cadc_readout_mode;

	bool m_has_plasticity_rule{false};

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
	 * Get whether input list is complete for the local execution instance.
	 * @return Boolean value
	 */
	bool has_complete_input_list() const;

	/**
	 * Filter events via batch entry runtime and recording interval.
	 * The input data is to be modified because of sorting in-place.
	 * @param filtered_data Filtered data per batch
	 * @param data Event sequence
	 * @return Event sequences split for the batch entries with relative chip times
	 */
	template <typename T>
	void filter_events(std::vector<std::vector<T>>& filtered_data, std::vector<T>&& data) const;
};

} // namespace grenade::vx::execution::detail
