#pragma once
#include <memory>
#include <vector>
#include <tbb/flow_graph.h>

#include "grenade/vx/execution/detail/connection_state_storage.h"
#include "grenade/vx/signal_flow/execution_instance_hooks.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input_data.h"
#include "grenade/vx/signal_flow/output_data.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"
#include "stadls/vx/v3/playback_program.h"

namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx

namespace grenade::vx::execution::backend {
struct InitializedConnection;
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
	    grenade::common::ExecutionInstanceID const& execution_instance,
	    halco::hicann_dls::vx::v3::DLSGlobal const& dls_global,
	    std::vector<std::reference_wrapper<lola::vx::v3::Chip const>> const& configs,
	    backend::InitializedConnection& connection,
	    ConnectionStateStorage& connection_state_storage,
	    signal_flow::ExecutionInstanceHooks& hooks) SYMBOL_VISIBLE;

	void operator()(tbb::flow::continue_msg) SYMBOL_VISIBLE;

	struct PeriodicCADCReadoutTimes
	{
		// Begin of the realtime section of the first realtime snippet, that uses periodic CADC
		// recording. Matches roughly the time point of the first recorded CADC sample in each batch
		// entry
		std::vector<haldls::vx::v3::Timer::Value> time_zero;

		// Begin of the realtime snippets that use cadc recording and are associated with the
		// according ExecutionInstanceBuilder
		std::vector<haldls::vx::v3::Timer::Value> interval_begin;

		// End of the realtime snippets that use cadc recording and are associated with the
		// according ExecutionInstanceBuilder
		std::vector<haldls::vx::v3::Timer::Value> interval_end;
	};

private:
	std::vector<signal_flow::OutputData>& data_maps;
	std::vector<std::reference_wrapper<signal_flow::InputData const>> const& input_data_maps;
	std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs;
	grenade::common::ExecutionInstanceID execution_instance;
	halco::hicann_dls::vx::v3::DLSGlobal dls_global;
	std::vector<std::reference_wrapper<lola::vx::v3::Chip const>> configs;
	backend::InitializedConnection& connection;
	ConnectionStateStorage& connection_state_storage;
	signal_flow::ExecutionInstanceHooks& hooks;
	log4cxx::LoggerPtr logger;
};

} // namespace grenade::vx::execution::detail
