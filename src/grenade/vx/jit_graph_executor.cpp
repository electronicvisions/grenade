#include "grenade/vx/jit_graph_executor.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <map>
#include <set>
#include <vector>
#include <boost/range/adaptor/map.hpp>
#include <log4cxx/logger.h>
#include <tbb/flow_graph.h>

#include "grenade/vx/config.h"
#include "grenade/vx/data_map.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/execution_instance_builder.h"
#include "grenade/vx/execution_instance_node.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/chip.h"
#include "haldls/vx/padi.h"
#include "haldls/vx/synapse_driver.h"
#include "hate/history_wrapper.h"
#include "hate/timer.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/playback_program_builder.h"
#include "stadls/vx/run.h"

namespace grenade::vx {

DataMap JITGraphExecutor::run(
    Graph const& graph,
    DataMap const& input_list,
    ExecutorMap const& executor_map,
    ConfigMap const& config_map)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx;
	hate::Timer const timer;

	check(graph, executor_map);

	auto const& execution_instance_graph = graph.get_execution_instance_graph();
	auto const& execution_instance_map = graph.get_execution_instance_map();

	// execution graph
	tbb::flow::graph execution_graph;
	// start trigger node
	tbb::flow::broadcast_node<tbb::flow::continue_msg> start(execution_graph);

	// execution instance builders
	std::unordered_map<Graph::vertex_descriptor, ExecutionInstanceBuilder>
	    execution_instance_builder_map;

	// execution nodes
	std::unordered_map<Graph::vertex_descriptor, tbb::flow::continue_node<tbb::flow::continue_msg>>
	    nodes;

	// global data map
	DataMap output_activation_map;

	std::map<halco::hicann_dls::vx::DLSGlobal, hxcomm::ConnectionTimeInfo>
	    connection_time_info_begin;
	for (auto const [dls, executor] : executor_map) {
		connection_time_info_begin.emplace(
		    dls, std::visit([](auto const& e) { return e.get_time_info(); }, executor));
	}

	// build execution nodes
	for (auto const vertex :
	     boost::make_iterator_range(boost::vertices(execution_instance_graph))) {
		auto const execution_instance = execution_instance_map.left.at(vertex);
		auto const dls_global = execution_instance.toDLSGlobal();
		execution_instance_builder_map.insert(std::make_pair(
		    vertex, ExecutionInstanceBuilder(
		                graph, execution_instance, input_list, output_activation_map,
		                config_map.at(dls_global))));
		auto& builder = execution_instance_builder_map.at(vertex);
		ExecutionInstanceNode node_body(
		    output_activation_map, builder, executor_map.at(dls_global));
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
	for (auto const& [dls, e] : executor_map) {
		auto const connection_time_info_difference =
		    std::visit([](auto const& c) { return c.get_time_info(); }, e) -
		    connection_time_info_begin.at(dls);
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

bool JITGraphExecutor::is_executable_on(Graph const& graph, ExecutorMap const& executor_map)
{
	auto const executor_dls_globals = boost::adaptors::keys(executor_map);
	auto const& execution_instance_map = graph.get_execution_instance_map();
	auto const& vertex_descriptor_map = graph.get_vertex_descriptor_map();

	auto const vertices = boost::make_iterator_range(boost::vertices(graph.get_graph()));
	return std::none_of(vertices.begin(), vertices.end(), [&](auto const vertex) {
		return std::find(
		           executor_dls_globals.begin(), executor_dls_globals.end(),
		           execution_instance_map.left.at(vertex_descriptor_map.left.at(vertex))
		               .toDLSGlobal()) == executor_dls_globals.end();
	});
}

void JITGraphExecutor::check(Graph const& graph, ExecutorMap const& executor_map)
{
	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
	hate::Timer const timer;

	// check execution instance graph is acyclic
	if (!graph.is_acyclic_execution_instance_graph()) {
		throw std::runtime_error("Execution instance graph is not acyclic.");
	}

	// check all DLSGlobal physical chips used in the graph are present in the provided executor map
	if (!is_executable_on(graph, executor_map)) {
		throw std::runtime_error("Graph requests executors not provided.");
	}

	LOG4CXX_TRACE(
	    logger,
	    "check(): Checked fit of graph, input list and executors in " << timer.print() << ".");
}

} // namespace grenade::vx
