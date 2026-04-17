#pragma once
#include "grenade/common/linked_topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/execution/detail/system.h"
#include "grenade/vx/ppu/neuron_view_handle.h"
#include "grenade/vx/ppu/synapse_array_view_handle.h"
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
#include <log4cxx/logger.h>

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
	 * @param topology Topology to use
	 * @param execution_instance_vertex_descriptor Vertex descriptor of execution instance to visit
	 * @param input_data Input data to visit
	 * @param chip_on_connection Chip on connection to visit for
	 */
	ExecutionInstanceChipSnippetConfigVisitor(
	    grenade::common::LinkedTopology const& topology,
	    grenade::common::VertexOnTopology const& execution_instance_vertex_descriptor,
	    grenade::common::InputData const& input_data,
	    common::ChipOnConnection const& chip_on_connection) SYMBOL_VISIBLE;

	/**
	 * Perform visit operation and generate initial configuration.
	 */
	void operator()(System& system) const SYMBOL_VISIBLE;

private:
	grenade::common::LinkedTopology const& m_topology;
	grenade::common::VertexOnTopology m_execution_instance_vertex_descriptor;
	grenade::common::InputData const& m_input_data;
	common::ChipOnConnection m_chip_on_connection;
	log4cxx::LoggerPtr m_logger;

	/**
	 * Process single vertex.
	 * This function is called in preprocess.
	 * @param vertex Vertex descriptor
	 * @param data Data associated with vertex
	 * @param chip Chip configuration to alter
	 */
	template <typename Vertex>
	void process(
	    grenade::common::VertexOnTopology const vertex, Vertex const& data, System& system) const;

	std::unordered_map<
	    std::type_index,
	    std::function<void(
	        grenade::common::VertexOnTopology const&, grenade::common::Vertex const&, System&)>>
	    m_visitor;

	template <typename Vertex>
	void register_process();
};

} // namespace grenade::vx::execution::detail
