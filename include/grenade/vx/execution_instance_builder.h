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
	 * @param graph Graph to use for property lookup
	 * @param input_list Input list to use for input data lookup
	 * @param data_output Data output from previous time steps to use for data lookup
	 * @param hemisphere Hemisphere of a chip to use for configuration placement
	 */
	ExecutionInstanceBuilder(
	    Graph const& graph,
	    DataMap const& input_list,
	    DataMap const& data_output,
	    halco::hicann_dls::vx::HemisphereOnDLS hemisphere,
	    HemisphereConfig const& hemisphere_config) SYMBOL_VISIBLE;

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

private:
	Graph const& m_graph;
	DataMap const& m_input_list;
	DataMap const& m_data_output;
	DataMap m_local_external_data;
	hate::HistoryWrapper<HemisphereConfig> m_config;
	stadls::vx::PlaybackProgramBuilder m_builder_input;
	stadls::vx::PlaybackProgramBuilder m_builder_neuron_reset;
	halco::hicann_dls::vx::HemisphereOnDLS m_hemisphere;
	std::vector<Graph::vertex_descriptor> m_post_vertices;
	bool m_postprocessing;

	DataMap m_local_data;
	DataMap m_local_data_output;

	typedef std::optional<stadls::vx::PlaybackProgram::ContainerTicket<lola::vx::CADCSampleRow>>
	    ticket_type;
	ticket_type m_ticket;
	ticket_type m_ticket_baseline;
	stadls::vx::PlaybackProgramBuilder m_builder_cadc_readout;
	stadls::vx::PlaybackProgramBuilder m_builder_cadc_readout_baseline;

	bool inputs_available(Graph::vertex_descriptor const descriptor) const SYMBOL_VISIBLE;

	/**
	 * Extract input activations for the given vertex.
	 * @param descriptor Vertex descriptor on the graph to extract for
	 */
	typename std::map<Graph::vertex_descriptor, std::vector<UInt5>>::const_reference
	get_input_activations(Graph::vertex_descriptor descriptor);
	std::vector<Int8> get_input_data(Graph::vertex_descriptor descriptor);

	/**
	 * Visit neuron columns for a given vertex with a given functor.
	 * @param vertex Vertex descriptor
	 * @param f Functor
	 */
	template <typename F>
	void visit_columns(Graph::vertex_descriptor const vertex, F&& f);

	std::unordered_map<size_t, halco::hicann_dls::vx::SynapseOnSynapseRow> get_column_map(
	    Graph::vertex_descriptor const vertex);

	void process(Graph::vertex_descriptor const vertex, vertex::SynapseArrayView const& data);

	void process(Graph::vertex_descriptor const vertex, vertex::DataInput const& data);

	void process(
	    Graph::vertex_descriptor const vertex, vertex::CADCMembraneReadoutView const& data);

	void process(Graph::vertex_descriptor const vertex, vertex::NeuronView const& data);

	void process(Graph::vertex_descriptor const vertex, vertex::ExternalInput const& data);

	void process(
	    Graph::vertex_descriptor const vertex, vertex::ConvertInt8ToSynapseInputLabel const& data);

	void process(Graph::vertex_descriptor const vertex, vertex::DataOutput const& data);

	void process(Graph::vertex_descriptor const vertex, vertex::Addition const& data);
};

} // namespace grenade::vx
