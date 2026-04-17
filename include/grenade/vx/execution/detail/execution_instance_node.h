#pragma once
#include <memory>
#include <vector>
#include <tbb/flow_graph.h>

#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/input_data.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/output_data.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/execution/execution_instance_hooks.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"
#include "stadls/vx/v3/playback_program.h"

namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx

namespace grenade::vx::execution::backend {
struct StatefulConnection;
} // namespace grenade::vx::execution::backend

namespace grenade::vx::execution::detail {

/**
 * Content of a execution node.
 * On invocation a node preprocesses a local part of the graph, builds a playback program for
 * execution, executes it and postprocesses result data.
 * Execution is triggered by an incoming message.
 */
struct ExecutionInstanceNode
{
	/**
	 * Construct execution instance node.
	 * @param output_data Output data to add results into
	 * @param results_mutex Mutex with which to guard modifying output_data
	 * @param topologies Topologies to use
	 * @param execution_instance Execution instance to execute
	 * @param data Input data to use
	 * @param connection Connection to device use
	 * @param hooks Execution instance hooks to use
	 * @param execution_instance_vertex_descriptors Vertex descriptors per realtime snippet of
	 * execution instance to visit
	 */
	ExecutionInstanceNode(
	    std::vector<grenade::common::OutputData>& output_data,
	    std::mutex& results_mutex,
	    std::vector<std::shared_ptr<grenade::common::LinkedTopology>> const& topologies,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance,
	    std::vector<std::reference_wrapper<grenade::common::InputData const>> const& data,
	    backend::StatefulConnection& connection,
	    ExecutionInstanceHooks& hooks,
	    std::vector<grenade::common::VertexOnTopology> const& execution_instance_vertex_descriptors)
	    SYMBOL_VISIBLE;

	void operator()(tbb::flow::continue_msg) SYMBOL_VISIBLE;

	struct PeriodicCADCReadoutTimes
	{
		// Begin of the realtime section of the first realtime snippet, that uses periodic CADC
		// recording. Matches roughly the time point of the first recorded CADC sample in each batch
		// entry
		std::vector<haldls::vx::v3::Timer::Value> time_zero;

		// Begin of the realtime snippets that use cadc recording and are associated with the
		// according ExecutionInstanceSnippetRealtimeExecutor
		std::vector<haldls::vx::v3::Timer::Value> interval_begin;

		// End of the realtime snippets that use cadc recording and are associated with the
		// according ExecutionInstanceSnippetRealtimeExecutor
		std::vector<haldls::vx::v3::Timer::Value> interval_end;
	};

private:
	std::vector<grenade::common::OutputData>& output_data;
	std::mutex& results_mutex;
	std::vector<std::shared_ptr<grenade::common::LinkedTopology>> const& topologies;
	grenade::common::ExecutionInstanceOnExecutor execution_instance;
	std::vector<std::reference_wrapper<grenade::common::InputData const>> input_data;
	backend::StatefulConnection& connection;
	ExecutionInstanceHooks& hooks;
	std::vector<grenade::common::VertexOnTopology> const& execution_instance_vertex_descriptors;
	log4cxx::LoggerPtr logger;
};

} // namespace grenade::vx::execution::detail
