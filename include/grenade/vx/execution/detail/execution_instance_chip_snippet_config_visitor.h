#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/ppu/neuron_view_handle.h"
#include "grenade/vx/ppu/synapse_array_view_handle.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"
#include "stadls/vx/v3/playback_program_builder.h"
#include <optional>
#include <tuple>
#include <vector>

namespace grenade::vx::execution::detail {

/**
 * Visitor of graph vertices of a single execution instance for construction of the initial
 * configuration.
 * The result is applied to a given configuration object.
 */
class ExecutionInstanceChipSnippetConfigVisitor
{
public:
	/**
	 * Construct visitor.
	 * @param graph Graph to use for locality and property lookup
	 * @param execution_instance Local execution instance to visit
	 */
	ExecutionInstanceChipSnippetConfigVisitor(
	    signal_flow::Graph const& graph,
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::ExecutionInstanceID const& execution_instance) SYMBOL_VISIBLE;

	/**
	 * Perform visit operation and generate initial configuration.
	 */
	void operator()(lola::vx::v3::Chip& chip) const SYMBOL_VISIBLE;

private:
	signal_flow::Graph const& m_graph;
	common::ChipOnConnection m_chip_on_connection;
	grenade::common::ExecutionInstanceID m_execution_instance;

	/**
	 * Process single vertex.
	 * This function is called in preprocess.
	 * @param vertex Vertex descriptor
	 * @param data Data associated with vertex
	 * @param chip Chip configuration to alter
	 */
	template <typename Vertex>
	void process(
	    signal_flow::Graph::vertex_descriptor const vertex,
	    Vertex const& data,
	    lola::vx::v3::Chip& chip) const;
};

} // namespace grenade::vx::execution::detail
