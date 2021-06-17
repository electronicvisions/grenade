#pragma once
#include <atomic>
#include <optional>
#include <set>
#include <vector>

#include "grenade/vx/config.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/execution_instance_config_builder.h"
#include "grenade/vx/execution_instance_playback_hooks.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/neuron_reset_mask_generator.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/v2/chip.h"
#include "haldls/vx/v2/ppu.h"
#include "haldls/vx/v2/synapse_driver.h"
#include "hate/visibility.h"
#include "lola/vx/v2/cadc.h"
#include "lola/vx/v2/synapse.h"
#include "stadls/vx/v2/playback_generator.h"
#include "stadls/vx/v2/playback_program.h"
#include "stadls/vx/v2/playback_program_builder.h"

namespace grenade::vx {

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
	    Graph const& graph,
	    coordinate::ExecutionInstance const& execution_instance,
	    IODataMap const& input_list,
	    IODataMap const& data_output,
	    ChipConfig const& chip_config,
	    ExecutionInstancePlaybackHooks& playback_hooks) SYMBOL_VISIBLE;

	/**
	 * Preprocess by single visit of all local vertices.
	 */
	void pre_process() SYMBOL_VISIBLE;

	/**
	 * Generate playback sequence.
	 * @return PlaybackPrograms generated via local graph traversal
	 */
	std::vector<stadls::vx::v2::PlaybackProgram> generate() SYMBOL_VISIBLE;

	/**
	 * Postprocess by visit of all local vertices to be post processed after execution.
	 * This resets the internal state of the builder to be ready for the next time step.
	 * @return IODataMap of locally computed results
	 */
	IODataMap post_process() SYMBOL_VISIBLE;
	void post_process(Graph::vertex_descriptor const vertex) SYMBOL_VISIBLE;

	/**
	 * Switch to enable CADC baseline read before each sent input vector.
	 * If disabled, the membrane resting potential is assumed to reside at CADC value 128.
	 */
	bool enable_cadc_baseline = true;

private:
	Graph const& m_graph;
	coordinate::ExecutionInstance m_execution_instance;
	IODataMap const& m_input_list;
	IODataMap const& m_data_output;

	ConstantReferenceIODataMap m_local_external_data;

	ExecutionInstanceConfigBuilder m_config_builder;

	ExecutionInstancePlaybackHooks& m_playback_hooks;

	std::vector<Graph::vertex_descriptor> m_post_vertices;

	std::vector<stadls::vx::v2::PlaybackProgram> m_chunked_program;

	std::optional<Graph::vertex_descriptor> m_event_input_vertex;
	std::optional<Graph::vertex_descriptor> m_event_output_vertex;

	bool m_postprocessing;

	IODataMap m_local_data;
	IODataMap m_local_data_output;

	typedef halco::common::typed_array<bool, halco::hicann_dls::vx::v2::HemisphereOnDLS>
	    ticket_request_type;
	ticket_request_type m_ticket_requests;

	struct BatchEntry
	{
		typedef halco::common::typed_array<
		    std::optional<
		        stadls::vx::v2::PlaybackProgram::ContainerTicket<haldls::vx::v2::PPUMemoryBlock>>,
		    halco::hicann_dls::vx::PPUOnDLS>
		    ticket_ppu_type;

		ticket_ppu_type m_ppu_result;

		typedef std::optional<
		    stadls::vx::v2::PlaybackProgram::ContainerTicket<haldls::vx::v2::NullPayloadReadable>>
		    event_guard_ticket_type;
		event_guard_ticket_type m_ticket_events_begin;
		event_guard_ticket_type m_ticket_events_end;
	};

	std::vector<BatchEntry> m_batch_entries;

	NeuronResetMaskGenerator m_neuron_resets;
	// Optional vertex descriptor of MADC readout if the execution instance contains such
	std::optional<Graph::vertex_descriptor> m_madc_readout_vertex;

	/**
	 * Check if any incoming vertex requires post processing.
	 * @param descriptor Vertex descriptor to check for
	 * @return Boolean value
	 */
	bool inputs_available(Graph::vertex_descriptor const descriptor) const SYMBOL_VISIBLE;

	/**
	 * Process single vertex.
	 * This function is called in both preprocess and postprocess depending on whether the vertex
	 * requires post-execution processing.
	 * @param vertex Vertex descriptor
	 * @param data Data associated with vertex
	 */
	template <typename Vertex>
	void process(Graph::vertex_descriptor const vertex, Vertex const& data);

	/**
	 * Get whether input list is complete for the local execution instance.
	 * @return Boolean value
	 */
	bool has_complete_input_list() const;

	/**
	 * Filter events via batch entry runtime and recording interval.
	 * The input data is to be modified because of sorting in-place.
	 * @param data Event sequence
	 * @return Event sequences split for the batch entries with relative chip times
	 */
	template <typename T>
	std::vector<std::vector<T>> filter_events(std::vector<T>& data) const;
};

} // namespace grenade::vx
