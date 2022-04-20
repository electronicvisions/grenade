#include "grenade/vx/jit_graph_executor.h"

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/config.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/execution_instance_node.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/io_data_list.h"
#include "grenade/vx/io_data_map.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/timer.h"
#include <chrono>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <variant>
#include <boost/range/adaptor/map.hpp>
#include <log4cxx/logger.h>
#include <tbb/flow_graph.h>

namespace grenade::vx {

IODataMap JITGraphExecutor::run(
    Graph const& graph,
    IODataMap const& input_list,
    Connections const& connections,
    ChipConfigs const& chip_configs)
{
	PlaybackHooks empty;
	return run(graph, input_list, connections, chip_configs, empty);
}

IODataMap JITGraphExecutor::run(
    Graph const& graph,
    IODataMap const& input_list,
    Connections const& connections,
    ChipConfigs const& chip_configs,
    PlaybackHooks& playback_hooks)
{
	using namespace halco::hicann_dls::vx::v3;
	hate::Timer const timer;

	check(graph, connections);

	auto const& execution_instance_graph = graph.get_execution_instance_graph();
	auto const& execution_instance_map = graph.get_execution_instance_map();

	// execution graph
	tbb::flow::graph execution_graph;
	// start trigger node
	tbb::flow::broadcast_node<tbb::flow::continue_msg> start(execution_graph);

	// execution nodes
	std::unordered_map<Graph::vertex_descriptor, tbb::flow::continue_node<tbb::flow::continue_msg>>
	    nodes;

	// global data map
	IODataMap output_activation_map;

	std::map<DLSGlobal, hxcomm::ConnectionTimeInfo> connection_time_info_begin;
	for (auto const [dls, connection] : connections) {
		connection_time_info_begin.emplace(dls, connection.get_time_info());
	}

	// connection mutex map
	std::map<DLSGlobal, std::mutex> continuous_chunked_program_execution_mutexes;
	for (auto const [dls, connection] : connections) {
		continuous_chunked_program_execution_mutexes.emplace(
		    std::piecewise_construct, std::make_tuple(dls), std::make_tuple());
	}

	// build execution nodes
	for (auto const vertex :
	     boost::make_iterator_range(boost::vertices(execution_instance_graph))) {
		auto const execution_instance = execution_instance_map.left.at(vertex);
		auto const dls_global = execution_instance.toDLSGlobal();
		ExecutionInstanceNode node_body(
		    output_activation_map, input_list, graph, execution_instance,
		    chip_configs.at(dls_global), connections.at(dls_global),
		    continuous_chunked_program_execution_mutexes.at(dls_global),
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

	std::chrono::nanoseconds total_time(timer.get_ns());
	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
	LOG4CXX_INFO(logger, "run(): Executed graph in " << hate::to_string(total_time) << ".");
	for (auto const& [dls, e] : connections) {
		auto const connection_time_info_difference =
		    e.get_time_info() - connection_time_info_begin.at(dls);
		LOG4CXX_INFO(
		    logger, "run(): Chip at "
		                << dls << " spent "
		                << hate::to_string(connection_time_info_difference.execution_duration)
		                << " in execution, which is "
		                << (static_cast<double>(
		                        connection_time_info_difference.execution_duration.count()) /
		                    static_cast<double>(total_time.count()) * 100.)
		                << " % of total graph execution time.");
	}

	return output_activation_map;
}

IODataList JITGraphExecutor::run(
    Graph const& graph,
    IODataList const& input_list,
    Connections const& connections,
    ChipConfigs const& chip_configs,
    bool only_unconnected_output)
{
	PlaybackHooks empty;
	return run(graph, input_list, connections, chip_configs, empty, only_unconnected_output);
}

IODataList JITGraphExecutor::run(
    Graph const& graph,
    IODataList const& input_list,
    Connections const& connections,
    ChipConfigs const& chip_configs,
    PlaybackHooks& playback_hooks,
    bool only_unconnected_output)
{
	auto const output_map =
	    run(graph, input_list.to_input_map(graph), connections, chip_configs, playback_hooks);
	IODataList output;
	output.from_output_map(output_map, graph, only_unconnected_output);
	return output;
}

bool JITGraphExecutor::is_executable_on(Graph const& graph, Connections const& connections)
{
	auto const connection_dls_globals = boost::adaptors::keys(connections);
	auto const& execution_instance_map = graph.get_execution_instance_map();
	auto const& vertex_descriptor_map = graph.get_vertex_descriptor_map();

	auto const vertices = boost::make_iterator_range(boost::vertices(graph.get_graph()));
	return std::none_of(vertices.begin(), vertices.end(), [&](auto const vertex) {
		return std::find(
		           connection_dls_globals.begin(), connection_dls_globals.end(),
		           execution_instance_map.left.at(vertex_descriptor_map.left.at(vertex))
		               .toDLSGlobal()) == connection_dls_globals.end();
	});
}

void JITGraphExecutor::check(Graph const& graph, Connections const& connections)
{
	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
	hate::Timer const timer;

	// check execution instance graph is acyclic
	if (!graph.is_acyclic_execution_instance_graph()) {
		throw std::runtime_error("Execution instance graph is not acyclic.");
	}

	// check all DLSGlobal physical chips used in the graph are present in the provided connections
	if (!is_executable_on(graph, connections)) {
		throw std::runtime_error("Graph requests connection not provided.");
	}

	LOG4CXX_TRACE(
	    logger,
	    "check(): Checked fit of graph, input list and connection in " << timer.print() << ".");
}

} // namespace grenade::vx
