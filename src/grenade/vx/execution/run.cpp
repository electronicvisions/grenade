#include "grenade/vx/execution/run.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/execution/backend/initialized_connection.h"
#include "grenade/vx/execution/detail/execution_instance_node.h"
#include "grenade/vx/signal_flow/execution_time_info.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input_data.h"
#include "grenade/vx/signal_flow/output_data.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/timer.h"
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <boost/range/adaptor/map.hpp>
#include <log4cxx/logger.h>
#include <tbb/flow_graph.h>

namespace grenade::vx::execution {

signal_flow::OutputData run(
    JITGraphExecutor& executor,
    signal_flow::Graph const& graph,
    JITGraphExecutor::ChipConfigs const& initial_config,
    signal_flow::InputData const& input,
    JITGraphExecutor::Hooks&& hooks)
{
	return std::move(
	    run(executor, std::vector<std::reference_wrapper<signal_flow::Graph const>>{graph},
	        {initial_config}, input, std::move(hooks)));
}

namespace {

bool value_equal(signal_flow::Graph::graph_type const& a, signal_flow::Graph::graph_type const& b)
{
	return std::equal(
	           boost::vertices(a).first, boost::vertices(a).second, boost::vertices(b).first,
	           boost::vertices(b).second) &&
	       std::equal(
	           boost::edges(a).first, boost::edges(a).second, boost::edges(b).first,
	           boost::edges(b).second, [&](auto const& aa, auto const& bb) {
		           return (boost::source(aa, a) == boost::source(bb, b)) &&
		                  (boost::target(aa, a) == boost::target(aa, b));
	           });
}

} // namespace

signal_flow::OutputData run(
    JITGraphExecutor& executor,
    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
    std::vector<std::reference_wrapper<JITGraphExecutor::ChipConfigs const>> const& configs,
    signal_flow::InputData const& inputs,
    JITGraphExecutor::Hooks&& hooks)
{
	// assure, that all vectors, which contain one element per realtime snippet are of the same size
	if (graphs.size() != inputs.snippets.size() || graphs.size() != configs.size()) {
		throw std::logic_error(
		    "Arguments 'graphs', 'inputs' and 'configs' must be of the same size");
	}
	// assure, that all vectors contain at least one element, otherwise the experiment is not
	// existent
	if (graphs.size() < 1) {
		throw std::logic_error(
		    "Arguments 'graphs', 'inputs' and 'configs' must contain at least one element");
	}

	using namespace halco::hicann_dls::vx::v3;
	hate::Timer const timer;

	auto const& execution_instance_graph = graphs[0].get().get_execution_instance_graph();
	auto const& execution_instance_map = graphs[0].get().get_execution_instance_map();

	// check, that all graphs have similar execution_instance_graphs/-maps and use the same chips
	executor.check(graphs[0]);
	for (size_t i = 1; i < graphs.size(); i++) {
		executor.check(graphs[i]);
		if (!value_equal(
		        execution_instance_graph, graphs[i].get().get_execution_instance_graph())) {
			throw std::runtime_error(
			    "graph corresponding to configuration " + std::to_string(i) +
			    " has differing execution_instance_graph from other graphs");
		}
		if (execution_instance_map != graphs[i].get().get_execution_instance_map()) {
			throw std::runtime_error(
			    "graph corresponding to configuration " + std::to_string(i) +
			    " has differing execution_instance_map from other graphs");
		}
	}
	std::map<signal_flow::Graph::vertex_descriptor, std::set<grenade::common::ConnectionOnExecutor>>
	    vps_per_execution_instance;
	for (auto const vertex :
	     boost::make_iterator_range(boost::vertices(execution_instance_graph))) {
		auto const get_vps = [graphs, vertex](size_t i) {
			std::set<grenade::common::ConnectionOnExecutor> vps;
			auto const& vertex_descriptor_map = graphs[i].get().get_vertex_descriptor_map();
			for (auto const& [vertex_descriptor, _] :
			     boost::make_iterator_range(vertex_descriptor_map.right.equal_range(vertex))) {
				auto const& vertex_property =
				    graphs[i].get().get_vertex_property(vertex_descriptor);
				std::visit(
				    [&vps](auto const& vp) {
					    if constexpr (std::is_base_of_v<
					                      signal_flow::vertex::EntityOnChip,
					                      std::decay_t<decltype(vp)>>) {
						    vps.insert(vp.chip_on_executor.second);
					    }
				    },
				    vertex_property);
			}
			return vps;
		};
		auto const vps = get_vps(0);
		vps_per_execution_instance[vertex] = vps;

		for (size_t i = 1; i < graphs.size(); i++) {
			// check, that all graphs use the same chips
			if (vps != get_vps(i)) {
				throw std::runtime_error(
				    "graph corresponding to configuration " + std::to_string(i) +
				    " uses other connections than other graphs");
			}
		}
	}


	// execution graph
	tbb::flow::graph execution_graph;
	// start trigger node
	tbb::flow::broadcast_node<tbb::flow::continue_msg> start(execution_graph);

	// execution nodes
	std::map<
	    signal_flow::Graph::vertex_descriptor, tbb::flow::continue_node<tbb::flow::continue_msg>>
	    nodes;

	// global data maps (each for one snippet)
	signal_flow::OutputData output_activation_maps;
	output_activation_maps.snippets.resize(graphs.size());

	// build execution nodes
	for (auto const vertex :
	     boost::make_iterator_range(boost::vertices(execution_instance_graph))) {
		auto const execution_instance = execution_instance_map.left.at(vertex);
		// collect all chips used by execution instance
		std::set<grenade::common::ConnectionOnExecutor>& connections_on_executor =
		    vps_per_execution_instance.at(vertex);
		// TODO: support execution instances without hardware usage
		if (connections_on_executor.empty()) {
			assert(executor.m_connections.size() > 0);
			connections_on_executor.insert(executor.m_connections.begin()->first);
		}
		if (connections_on_executor.size() > 1) {
			throw std::runtime_error(
			    "Execution instance using multiple connections not supported.");
		}
		assert(connections_on_executor.size() == 1);
		auto const connection_on_executor = *connections_on_executor.begin();

		// vector of configs to pass to execution_instance_node
		std::vector<
		    std::map<common::ChipOnConnection, std::reference_wrapper<lola::vx::v3::Chip const>>>
		    execution_node_configs(configs.size());
		for (size_t i = 0; i < configs.size(); i++) {
			for (auto const& [chip_on_connection, config] :
			     configs[i].get().at(execution_instance)) {
				execution_node_configs.at(i).emplace(chip_on_connection, config);
			}
		}
		if (!hooks.contains(execution_instance)) {
			hooks[execution_instance] = std::make_shared<signal_flow::ExecutionInstanceHooks>();
			for (auto const& [chip_on_connection, _] : configs.at(0).get().at(execution_instance)) {
				hooks.at(execution_instance)->chips[chip_on_connection] = {};
			}
		} else {
			for (auto const& [chip_on_connection, _] : configs.at(0).get().at(execution_instance)) {
				if (!hooks.at(execution_instance)->chips.contains(chip_on_connection)) {
					hooks.at(execution_instance)->chips[chip_on_connection] = {};
				}
			}
		}
		detail::ExecutionInstanceNode node_body(
		    output_activation_maps, inputs, graphs, execution_instance, connection_on_executor,
		    std::move(execution_node_configs), executor.m_connections.at(connection_on_executor),
		    *(hooks[execution_instance]));
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
	std::chrono::nanoseconds execution_duration(timer.get_ns());

	signal_flow::ExecutionTimeInfo execution_time_info;
	execution_time_info.execution_duration = execution_duration;
	if (output_activation_maps.execution_time_info) {
		output_activation_maps.execution_time_info->merge(execution_time_info);
	} else {
		output_activation_maps.execution_time_info = execution_time_info;
	}
	if (!output_activation_maps.execution_health_info) {
		output_activation_maps.execution_health_info = signal_flow::ExecutionHealthInfo();
	}

	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
	LOG4CXX_INFO(
	    logger,
	    "run(): Executed graph in "
	        << hate::to_string(output_activation_maps.execution_time_info->execution_duration)
	        << ".");
	for (auto const& [connection_on_executor, duration_per_chip] :
	     output_activation_maps.execution_time_info->execution_duration_per_hardware) {
		for (auto const& [chip_on_connection, duration] : duration_per_chip) {
			LOG4CXX_INFO(
			    logger, "run(): Chip at "
			                << connection_on_executor << ", " << chip_on_connection << " spent "
			                << hate::to_string(duration) << " in execution, which is "
			                << (static_cast<double>(duration.count()) /
			                    static_cast<double>(output_activation_maps.execution_time_info
			                                            ->execution_duration.count()) *
			                    100.)
			                << " % of total graph execution time.");
		}
	}
	if (output_activation_maps.execution_health_info) {
		LOG4CXX_TRACE(logger, "run(): " << *(output_activation_maps.execution_health_info) << ".");
	}

	return output_activation_maps;
}

} // namespace grenade::vx::execution
