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
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/execution_instance_builder.h"
#include "grenade/vx/execution_instance_node.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/v2/chip.h"
#include "haldls/vx/v2/padi.h"
#include "haldls/vx/v2/synapse_driver.h"
#include "hate/history_wrapper.h"
#include "hate/timer.h"
#include "stadls/vx/v2/playback_generator.h"
#include "stadls/vx/v2/playback_program_builder.h"
#include "stadls/vx/v2/run.h"

namespace grenade::vx {

IODataMap JITGraphExecutor::run(
    Graph const& graph,
    IODataMap const& input_list,
    Connections const& connections,
    ChipConfigs const& chip_configs)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;
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

	std::map<halco::hicann_dls::vx::v2::DLSGlobal, hxcomm::ConnectionTimeInfo>
	    connection_time_info_begin;
	for (auto const [dls, connection] : connections) {
		connection_time_info_begin.emplace(
		    dls, std::visit([](auto const& c) { return c.get_time_info(); }, connection));
	}

	// build execution nodes
	for (auto const vertex :
	     boost::make_iterator_range(boost::vertices(execution_instance_graph))) {
		auto const execution_instance = execution_instance_map.left.at(vertex);
		auto const dls_global = execution_instance.toDLSGlobal();
		ExecutionInstanceNode node_body(
		    output_activation_map, input_list, graph, execution_instance,
		    chip_configs.at(dls_global), connections.at(dls_global));
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
