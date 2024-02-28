#pragma once
#include <memory>
#include <vector>
#include <tbb/flow_graph.h>

#include "grenade/vx/execution/detail/connection_state_storage.h"
#include "grenade/vx/execution/detail/execution_instance_builder.h"
#include "grenade/vx/signal_flow/execution_instance_hooks.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input_data.h"
#include "grenade/vx/signal_flow/output_data.h"
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
	    std::vector<signal_flow::OutputData>& data_maps,
	    std::vector<std::reference_wrapper<signal_flow::InputData const>> const& input_data_maps,
	    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
	    common::ExecutionInstanceID const& execution_instance,
	    halco::hicann_dls::vx::v3::DLSGlobal const& dls_global,
	    std::vector<std::reference_wrapper<lola::vx::v3::Chip const>> const& configs,
	    backend::Connection& connection,
	    ConnectionStateStorage& connection_state_storage,
	    signal_flow::ExecutionInstanceHooks& hooks) SYMBOL_VISIBLE;

	void operator()(tbb::flow::continue_msg) SYMBOL_VISIBLE;

private:
	std::vector<signal_flow::OutputData>& data_maps;
	std::vector<std::reference_wrapper<signal_flow::InputData const>> const& input_data_maps;
	std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs;
	common::ExecutionInstanceID execution_instance;
	halco::hicann_dls::vx::v3::DLSGlobal dls_global;
	std::vector<std::reference_wrapper<lola::vx::v3::Chip const>> configs;
	backend::Connection& connection;
	ConnectionStateStorage& connection_state_storage;
	signal_flow::ExecutionInstanceHooks& hooks;
	log4cxx::LoggerPtr logger;

	struct PlaybackPrograms
	{
		std::vector<stadls::vx::v3::PlaybackProgram> realtime;
		bool has_hook_around_realtime;
		bool has_plasticity;
	};
};

} // namespace grenade::vx::execution::detail
