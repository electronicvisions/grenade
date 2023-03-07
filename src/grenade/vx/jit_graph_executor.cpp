#include "grenade/vx/jit_graph_executor.h"

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/execution_instance_node.h"
#include "grenade/vx/execution_time_info.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/io_data_list.h"
#include "grenade/vx/io_data_map.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/timer.h"
#include "hxcomm/vx/connection_from_env.h"
#include <chrono>
#include <map>
#include <sstream>
#include <stdexcept>
#include <variant>
#include <boost/range/adaptor/map.hpp>
#include <log4cxx/logger.h>
#include <tbb/flow_graph.h>

namespace grenade::vx {

JITGraphExecutor::JITGraphExecutor(bool const enable_differential_config) :
    m_connections(),
    m_connection_state_storages(),
    m_enable_differential_config(enable_differential_config)
{
	auto hxcomm_connections = hxcomm::vx::get_connection_list_from_env();
	for (size_t i = 0; i < hxcomm_connections.size(); ++i) {
		halco::hicann_dls::vx::v3::DLSGlobal identifier(i);
		m_connections.emplace(
		    identifier, std::move(backend::Connection(std::move(hxcomm_connections.at(i)))));
		m_connection_state_storages.emplace(
		    std::piecewise_construct, std::forward_as_tuple(identifier),
		    std::forward_as_tuple(m_enable_differential_config, m_connections.at(identifier)));
	}
}

JITGraphExecutor::JITGraphExecutor(
    std::map<halco::hicann_dls::vx::v3::DLSGlobal, backend::Connection>&& connections,
    bool const enable_differential_config) :
    m_connections(std::move(connections)),
    m_connection_state_storages(),
    m_enable_differential_config(enable_differential_config)
{
	for (auto const& [identifier, _] : m_connections) {
		m_connection_state_storages.emplace(
		    std::piecewise_construct, std::forward_as_tuple(identifier),
		    std::forward_as_tuple(m_enable_differential_config, m_connections.at(identifier)));
	}
}

std::set<halco::hicann_dls::vx::v3::DLSGlobal> JITGraphExecutor::contained_connections() const
{
	std::set<halco::hicann_dls::vx::v3::DLSGlobal> ret;
	for (auto const& [identifier, _] : m_connections) {
		ret.insert(identifier);
	}
	return ret;
}

std::map<halco::hicann_dls::vx::v3::DLSGlobal, backend::Connection>&&
JITGraphExecutor::release_connections()
{
	m_connection_state_storages.clear();
	return std::move(m_connections);
}

std::map<halco::hicann_dls::vx::v3::DLSGlobal, hxcomm::ConnectionTimeInfo>
JITGraphExecutor::get_time_info() const
{
	std::map<halco::hicann_dls::vx::v3::DLSGlobal, hxcomm::ConnectionTimeInfo> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_time_info());
	}
	return ret;
}

std::map<halco::hicann_dls::vx::v3::DLSGlobal, std::string> JITGraphExecutor::get_unique_identifier(
    std::optional<std::string> const& hwdb_path) const
{
	std::map<halco::hicann_dls::vx::v3::DLSGlobal, std::string> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_unique_identifier(hwdb_path));
	}
	return ret;
}

std::map<halco::hicann_dls::vx::v3::DLSGlobal, std::string> JITGraphExecutor::get_bitfile_info()
    const
{
	std::map<halco::hicann_dls::vx::v3::DLSGlobal, std::string> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_bitfile_info());
	}
	return ret;
}

std::map<halco::hicann_dls::vx::v3::DLSGlobal, std::string>
JITGraphExecutor::get_remote_repo_state() const
{
	std::map<halco::hicann_dls::vx::v3::DLSGlobal, std::string> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_remote_repo_state());
	}
	return ret;
}

bool JITGraphExecutor::is_executable_on(Graph const& graph)
{
	auto const connection_dls_globals = boost::adaptors::keys(m_connections);
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

void JITGraphExecutor::check(Graph const& graph)
{
	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
	hate::Timer const timer;

	// check execution instance graph is acyclic
	if (!graph.is_acyclic_execution_instance_graph()) {
		throw std::runtime_error("Execution instance graph is not acyclic.");
	}

	// check all DLSGlobal physical chips used in the graph are present in the provided connections
	if (!is_executable_on(graph)) {
		throw std::runtime_error("Graph requests connection not provided.");
	}

	LOG4CXX_TRACE(
	    logger,
	    "check(): Checked fit of graph, input list and connection in " << timer.print() << ".");
}


IODataMap run(
    JITGraphExecutor& executor,
    Graph const& graph,
    IODataMap const& input,
    JITGraphExecutor::ChipConfigs const& initial_config)
{
	JITGraphExecutor::PlaybackHooks empty;
	return run(executor, graph, input, initial_config, empty);
}

IODataMap run(
    JITGraphExecutor& executor,
    Graph const& graph,
    IODataMap const& input,
    JITGraphExecutor::ChipConfigs const& initial_config,
    JITGraphExecutor::PlaybackHooks& playback_hooks)
{
	using namespace halco::hicann_dls::vx::v3;
	hate::Timer const timer;

	executor.check(graph);

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

	// build execution nodes
	for (auto const vertex :
	     boost::make_iterator_range(boost::vertices(execution_instance_graph))) {
		auto const execution_instance = execution_instance_map.left.at(vertex);
		auto const dls_global = execution_instance.toDLSGlobal();
		ExecutionInstanceNode node_body(
		    output_activation_map, input, graph, execution_instance,
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

	ExecutionTimeInfo execution_time_info;
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

IODataList run(
    JITGraphExecutor& executor,
    Graph const& graph,
    IODataList const& input,
    JITGraphExecutor::ChipConfigs const& initial_config,
    bool only_unconnected_output)
{
	JITGraphExecutor::PlaybackHooks empty;
	return run(executor, graph, input, initial_config, empty, only_unconnected_output);
}

IODataList run(
    JITGraphExecutor& executor,
    Graph const& graph,
    IODataList const& input,
    JITGraphExecutor::ChipConfigs const& initial_config,
    JITGraphExecutor::PlaybackHooks& playback_hooks,
    bool only_unconnected_output)
{
	auto const output_map =
	    run(executor, graph, input.to_input_map(graph), initial_config, playback_hooks);
	IODataList output;
	output.from_output_map(output_map, graph, only_unconnected_output);
	return output;
}

} // namespace grenade::vx
