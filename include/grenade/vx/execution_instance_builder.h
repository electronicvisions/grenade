#pragma once
#include <atomic>
#include <optional>
#include <set>
#include <vector>

#include "grenade/vx/config.h"
#include "grenade/vx/data_map.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/chip.h"
#include "haldls/vx/synapse_driver.h"
#include "hate/history_wrapper.h"
#include "hate/visibility.h"
#include "lola/vx/cadc.h"
#include "lola/vx/synapse.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/playback_program_builder.h"

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
	 * @param input_list Input list to use for input data lookup
	 * @param data_output Data output from depended-on executions to use for data lookup
	 * @param chip_config Chip configuration to use
	 */
	ExecutionInstanceBuilder(
	    Graph const& graph,
	    DataMap const& input_list,
	    DataMap const& data_output,
	    ChipConfig const& chip_config) SYMBOL_VISIBLE;

	/**
	 * Process a vertex.
	 * Visits data properties linked to the vertex and embodies static configuration, e.g. synaptic
	 * weights, as well as registers runtime configuration, e.g. events.
	 * @param vertex Vertex descriptor
	 */
	void process(Graph::vertex_descriptor const vertex) SYMBOL_VISIBLE;

	/**
	 * Generate a playback sequence and result handle.
	 * This resets the internal state of the builder to be ready for the next time step.
	 */
	stadls::vx::PlaybackProgramBuilder generate() SYMBOL_VISIBLE;

	/**
	 * Postprocess by visit of all local vertices to be post processed after execution.
	 * This resets the internal state of the builder to be ready for the next time step.
	 * @return DataMap of locally computed results
	 */
	DataMap post_process() SYMBOL_VISIBLE;
	void post_process(Graph::vertex_descriptor const vertex) SYMBOL_VISIBLE;

	/**
	 * Switch to enable HAGEN-mode workarounds needed for HXv1.
	 * TODO: remove once HXv2 is working.
	 */
	bool enable_hagen_workarounds = true;

private:
	Graph const& m_graph;
	DataMap const& m_input_list;
	DataMap const& m_data_output;
	DataMap m_local_external_data;
	hate::HistoryWrapper<ChipConfig> m_config;
	stadls::vx::PlaybackProgramBuilder m_builder_input;
	stadls::vx::PlaybackProgramBuilder m_builder_neuron_reset;
	std::vector<Graph::vertex_descriptor> m_post_vertices;
	bool m_postprocessing;

	DataMap m_local_data;
	DataMap m_local_data_output;

	typedef halco::common::typed_array<bool, halco::hicann_dls::vx::HemisphereOnDLS>
	    ticket_request_type;
	ticket_request_type m_ticket_requests;
	typedef std::optional<stadls::vx::PlaybackProgram::ContainerTicket<lola::vx::CADCSamples>>
	    ticket_type;
	ticket_type m_ticket;
	ticket_type m_ticket_baseline;
	stadls::vx::PlaybackProgramBuilder m_builder_cadc_readout;
	stadls::vx::PlaybackProgramBuilder m_builder_cadc_readout_baseline;

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

	halco::common::typed_array<bool, halco::hicann_dls::vx::NeuronResetOnDLS> m_neuron_resets;
	halco::common::typed_array<bool, halco::hicann_dls::vx::PADIBusOnDLS> m_used_padi_busses;

	// TODO: remove once Billy-bug is resolved
	halco::common::
	    typed_array<lola::vx::SynapseMatrix::Label, halco::hicann_dls::vx::SynapseRowOnDLS>
	        m_hagen_addresses;
};

} // namespace grenade::vx
