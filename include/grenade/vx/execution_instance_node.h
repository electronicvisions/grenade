#pragma once
#include <tbb/flow_graph.h>

#include "grenade/vx/data_map.h"
#include "grenade/vx/execution_instance_builder.h"
#include "grenade/vx/graph.h"
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
	    DataMap& data_map,
	    ExecutionInstanceBuilder& builder,
	    hxcomm::vx::ConnectionVariant& connection) SYMBOL_VISIBLE;

	void operator()(tbb::flow::continue_msg) SYMBOL_VISIBLE;

private:
	DataMap& data_map;
	ExecutionInstanceBuilder& builder;
	hxcomm::vx::ConnectionVariant& connection;
	log4cxx::Logger* logger;
};

} // namespace grenade::vx
