#pragma once
#include "grenade/vx/config.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/visibility.h"
#include "stadls/vx/v3/playback_program_builder.h"
#include <optional>
#include <tuple>

namespace grenade::vx {

/**
 * Builder for the static chip configuration of a single ExecutionInstance.
 * Vertices are processed resulting in a playback builder.
 */
class ExecutionInstanceConfigBuilder
{
public:
	/**
	 * Construct builder.
	 * @param graph Graph to use for locality and property lookup
	 * @param execution_instance Local execution instance to build for
	 * @param chip_config Chip configuration to use
	 */
	ExecutionInstanceConfigBuilder(
	    Graph const& graph,
	    coordinate::ExecutionInstance const& execution_instance,
	    ChipConfig const& chip_config) SYMBOL_VISIBLE;

	/**
	 * Preprocess by single visit of all local vertices.
	 */
	void pre_process() SYMBOL_VISIBLE;

	/**
	 * Generate playback program builder.
	 * @return PlaybackProgramBuilder generated via local graph traversal and optional PPU program
	 * symbols
	 */
	std::tuple<
	    stadls::vx::v3::PlaybackProgramBuilder,
	    std::optional<lola::vx::v3::PPUElfFile::symbols_type>>
	generate() SYMBOL_VISIBLE;

private:
	Graph const& m_graph;
	coordinate::ExecutionInstance m_execution_instance;

	ChipConfig m_config;

	halco::common::typed_array<bool, halco::hicann_dls::vx::v3::NeuronResetOnDLS>
	    m_enabled_neuron_resets;
	bool m_requires_ppu;
	bool m_used_madc;

	/**
	 * Process single vertex.
	 * This function is called in preprocess.
	 * @param vertex Vertex descriptor
	 * @param data Data associated with vertex
	 */
	template <typename Vertex>
	void process(Graph::vertex_descriptor const vertex, Vertex const& data);
};

} // namespace grenade::vx
