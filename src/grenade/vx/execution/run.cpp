#include "grenade/vx/execution/run.h"

#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/detail/execution_instance_node.h"
#include "grenade/vx/signal_flow/execution_time_info.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/io_data_list.h"
#include "grenade/vx/signal_flow/io_data_map.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/timer.h"
#include <chrono>
#include <map>
#include <mutex>
#include <stdexcept>
#include <boost/range/adaptor/map.hpp>
#include <log4cxx/logger.h>
#include <tbb/flow_graph.h>

namespace grenade::vx::execution {

signal_flow::IODataMap run(
    JITGraphExecutor& executor,
    signal_flow::Graph const& graph,
    signal_flow::IODataMap const& input,
    JITGraphExecutor::ChipConfigs const& initial_config)
{
	JITGraphExecutor::PlaybackHooks empty;
	return run(executor, graph, input, initial_config, empty);
}

signal_flow::IODataMap run(
    JITGraphExecutor& executor,
    signal_flow::Graph const& graph,
    signal_flow::IODataMap const& input,
    JITGraphExecutor::ChipConfigs const& initial_config,
    JITGraphExecutor::PlaybackHooks& playback_hooks)
{
	using namespace halco::hicann_dls::vx::v3;
	hate::Timer const timer;

	executor.check(graph);

	auto const& execution_instance_graph = graph.get_execution_instance_graph();
	auto const& execution_instance_map = graph.get_execution_instance_map();
	auto const& vertex_descriptor_map = graph.get_vertex_descriptor_map();

	// execution graph
	tbb::flow::graph execution_graph;
	// start trigger node
	tbb::flow::broadcast_node<tbb::flow::continue_msg> start(execution_graph);

	// execution nodes
	std::map<
	    signal_flow::Graph::vertex_descriptor, tbb::flow::continue_node<tbb::flow::continue_msg>>
	    nodes;

	// global data map
	signal_flow::IODataMap output_activation_map;

	// build execution nodes
	for (auto const vertex :
	     boost::make_iterator_range(boost::vertices(execution_instance_graph))) {
		auto const execution_instance = execution_instance_map.left.at(vertex);
		// collect all chips used by execution instance
		std::set<DLSGlobal> dls_globals;
		for (auto const& [_, vertex_descriptor] :
		     boost::make_iterator_range(vertex_descriptor_map.right.equal_range(vertex))) {
			auto const& vertex_property = graph.get_vertex_property(vertex_descriptor);
			std::visit(
			    [&dls_globals](auto const& vp) {
				    if constexpr (std::is_base_of_v<
				                      signal_flow::vertex::EntityOnChip,
				                      std::decay_t<decltype(vp)>>) {
					    dls_globals.insert(vp.get_coordinate_chip());
				    }
			    },
			    vertex_property);
		}
		// TODO: support execution instances without hardware usage
		if (dls_globals.empty()) {
			assert(executor.m_connection_state_storages.size() > 0);
			dls_globals.insert(executor.m_connection_state_storages.begin()->first);
		}
		if (dls_globals.size() > 1) {
			throw std::runtime_error(
			    "Execution instance using multiple chips not supported (yet).");
		}
		assert(dls_globals.size() == 1);
		auto const dls_global = *dls_globals.begin();
		detail::ExecutionInstanceNode node_body(
		    output_activation_map, input, graph, execution_instance, dls_global,
		    initial_config.at(execution_instance), executor.m_connections.at(dls_global),
		    executor.m_connection_state_storages.at(dls_global),
		    playback_hooks[execution_instance]);
		nodes.insert(std::make_pair(
		    vertex, tbb::flow::continue_node<tbb::flow::continue_msg>(execution_graph, node_body)));
	}

	// connect execution nodes
	for (auto const vertex :
	     boost::make_iterator_range(boost::vertices(execution_instance_graph))) {
		if (boost::in_degree(vertex, execution_instance_graph) == 0) {
			tbb::flow::make_edge(start, nodes.at(vertex));
		}
		for (auto const out_edge :
		     boost::make_iterator_range(boost::out_edges(vertex, execution_instance_graph))) {
			auto const target = boost::target(out_edge, execution_instance_graph);
			tbb::flow::make_edge(nodes.at(vertex), nodes.at(target));
		}
	}

	// trigger execution and wait for completion
	start.try_put(tbb::flow::continue_msg());
	execution_graph.wait_for_all();

	signal_flow::ExecutionTimeInfo execution_time_info;
	execution_time_info.execution_duration = std::chrono::nanoseconds(timer.get_ns());
	if (output_activation_map.execution_time_info) {
		output_activation_map.execution_time_info->merge(execution_time_info);
	} else {
		output_activation_map.execution_time_info = execution_time_info;
	}
	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
	LOG4CXX_INFO(
	    logger,
	    "run(): Executed graph in "
	        << hate::to_string(output_activation_map.execution_time_info->execution_duration)
	        << ".");
	for (auto const& [dls, duration] :
	     output_activation_map.execution_time_info->execution_duration_per_hardware) {
		LOG4CXX_INFO(
		    logger, "run(): Chip at "
		                << dls << " spent " << hate::to_string(duration)
		                << " in execution, which is "
		                << (static_cast<double>(duration.count()) /
		                    static_cast<double>(output_activation_map.execution_time_info
		                                            ->execution_duration.count()) *
		                    100.)
		                << " % of total graph execution time.");
	}
	output_activation_map.runtime = input.runtime;
	return output_activation_map;
}

signal_flow::IODataList run(
    JITGraphExecutor& executor,
    signal_flow::Graph const& graph,
    signal_flow::IODataList const& input,
    JITGraphExecutor::ChipConfigs const& initial_config,
    bool only_unconnected_output)
{
	JITGraphExecutor::PlaybackHooks empty;
	return run(executor, graph, input, initial_config, empty, only_unconnected_output);
}

signal_flow::IODataList run(
    JITGraphExecutor& executor,
    signal_flow::Graph const& graph,
    signal_flow::IODataList const& input,
    JITGraphExecutor::ChipConfigs const& initial_config,
    JITGraphExecutor::PlaybackHooks& playback_hooks,
    bool only_unconnected_output)
{
	auto const output_map =
	    run(executor, graph, input.to_input_map(graph), initial_config, playback_hooks);
	signal_flow::IODataList output;
	output.from_output_map(output_map, graph, only_unconnected_output);
	return output;
}

} // namespace grenade::vx::execution
