#pragma once
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "halco/hicann-dls/vx/v2/chip.h"
#include "halco/hicann-dls/vx/v2/neuron.h"
#include "hate/visibility.h"
#include "lola/vx/v2/chip.h"
#include "stadls/vx/v2/playback_program_builder.h"
#include <optional>
#include <tuple>

namespace grenade::vx {

/**
 * Builder for the static chip configuration of a single ExecutionInstance.
 * Vertices are processed resulting in a chip configuration.
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
	    lola::vx::v2::Chip const& chip_config) SYMBOL_VISIBLE;

	/**
	 * Preprocess by single visit of all local vertices.
	 */
	void pre_process() SYMBOL_VISIBLE;

	/**
	 * Generate static configuration.
	 * @return Chip object generated via local graph traversal and optional PPU program
	 * symbols
	 */
	std::tuple<lola::vx::v2::Chip, std::optional<lola::vx::v2::PPUElfFile::symbols_type>> generate()
	    SYMBOL_VISIBLE;

private:
	Graph const& m_graph;
	coordinate::ExecutionInstance m_execution_instance;

	lola::vx::v2::Chip m_config;

	halco::common::typed_array<bool, halco::hicann_dls::vx::v2::NeuronResetOnDLS>
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
