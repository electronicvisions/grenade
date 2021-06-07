#pragma once
#include <mutex>
#include <tbb/flow_graph.h>

#include "grenade/vx/execution_instance_builder.h"
#include "grenade/vx/execution_instance_playback_hooks.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/io_data_map.h"
#include "hate/visibility.h"
#include "hxcomm/vx/connection_variant.h"

namespace log4cxx {
class Logger;
} // namespace log4cxx

namespace grenade::vx {

/**
 * Content of a execution node.
 * On invokation a node preprocesses a local part of the graph, builds a playback program for
 * execution, executes it and postprocesses result data.
 * Execution is triggered by an incoming message.
 */
struct ExecutionInstanceNode
{
	ExecutionInstanceNode(
	    IODataMap& data_map,
	    IODataMap const& input_data_map,
	    Graph const& graph,
	    coordinate::ExecutionInstance const& execution_instance,
	    ChipConfig const& chip_config,
	    hxcomm::vx::ConnectionVariant& connection,
	    std::mutex& continuous_chunked_program_execution_mutex,
	    ExecutionInstancePlaybackHooks& playback_hooks) SYMBOL_VISIBLE;

	void operator()(tbb::flow::continue_msg) SYMBOL_VISIBLE;

private:
	IODataMap& data_map;
	IODataMap const& input_data_map;
	Graph const& graph;
	coordinate::ExecutionInstance execution_instance;
	ChipConfig const& chip_config;
	hxcomm::vx::ConnectionVariant& connection;
	std::mutex& continuous_chunked_program_execution_mutex;
	ExecutionInstancePlaybackHooks& playback_hooks;
	log4cxx::Logger* logger;
};

} // namespace grenade::vx
