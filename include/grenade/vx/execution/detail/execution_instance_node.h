#pragma once
#include <memory>
#include <tbb/flow_graph.h>

#include "grenade/vx/execution/detail/connection_state_storage.h"
#include "grenade/vx/execution/detail/execution_instance_builder.h"
#include "grenade/vx/signal_flow/execution_instance_playback_hooks.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/io_data_map.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"

namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx

namespace grenade::vx::execution::backend {
struct Connection;
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
	ExecutionInstanceNode(
	    signal_flow::IODataMap& data_map,
	    signal_flow::IODataMap const& input_data_map,
	    signal_flow::Graph const& graph,
	    common::ExecutionInstanceID const& execution_instance,
	    lola::vx::v3::Chip const& initial_config,
	    backend::Connection& connection,
	    ConnectionStateStorage& connection_state_storage,
	    signal_flow::ExecutionInstancePlaybackHooks& playback_hooks) SYMBOL_VISIBLE;

	void operator()(tbb::flow::continue_msg) SYMBOL_VISIBLE;

private:
	signal_flow::IODataMap& data_map;
	signal_flow::IODataMap const& input_data_map;
	signal_flow::Graph const& graph;
	common::ExecutionInstanceID execution_instance;
	lola::vx::v3::Chip const& initial_config;
	backend::Connection& connection;
	ConnectionStateStorage& connection_state_storage;
	signal_flow::ExecutionInstancePlaybackHooks& playback_hooks;
	log4cxx::LoggerPtr logger;
};

} // namespace grenade::vx::execution::detail
