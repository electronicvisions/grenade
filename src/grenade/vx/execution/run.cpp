#include "grenade/vx/execution/run.h"

#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/detail/execution_instance_node.h"
#include "grenade/vx/signal_flow/execution_time_info.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input_data.h"
#include "grenade/vx/signal_flow/io_data_list.h"
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
    signal_flow::InputData const& input,
    JITGraphExecutor::ChipConfigs const& initial_config)
{
	JITGraphExecutor::PlaybackHooks empty;
	return run(executor, graph, input, initial_config, empty);
}

std::vector<signal_flow::OutputData> run(
    JITGraphExecutor& executor,
    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
    std::vector<std::reference_wrapper<signal_flow::InputData const>> const& inputs,
    std::vector<std::reference_wrapper<JITGraphExecutor::ChipConfigs const>> const& configs)
{
	JITGraphExecutor::PlaybackHooks empty;
	return run(executor, graphs, inputs, configs, empty);
}

signal_flow::OutputData run(
    JITGraphExecutor& executor,
    signal_flow::Graph const& graph,
    signal_flow::InputData const& input,
    JITGraphExecutor::ChipConfigs const& initial_config,
    JITGraphExecutor::PlaybackHooks& playback_hooks)
{
	return std::move(run(executor,
	                     std::vector<std::reference_wrapper<signal_flow::Graph const>>{graph},
	                     {input}, {initial_config}, playback_hooks)
	                     .at(0));
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

std::vector<signal_flow::OutputData> run(
    JITGraphExecutor& executor,
    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
    std::vector<std::reference_wrapper<signal_flow::InputData const>> const& inputs,
    std::vector<std::reference_wrapper<JITGraphExecutor::ChipConfigs const>> const& configs,
    JITGraphExecutor::PlaybackHooks& playback_hooks)
{
	// assure, that all vectors, which contain one element per realtime column are of the same size
	if (graphs.size() != inputs.size() || graphs.size() != configs.size()) {
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
	std::map<signal_flow::Graph::vertex_descriptor, std::set<DLSGlobal>> vps_per_execution_instance;
	for (auto const vertex :
	     boost::make_iterator_range(boost::vertices(execution_instance_graph))) {
		auto const get_vps = [graphs, vertex](size_t i) {
			std::set<DLSGlobal> vps;
			auto const& vertex_descriptor_map = graphs[i].get().get_vertex_descriptor_map();
			for (auto const& [_, vertex_descriptor] :
			     boost::make_iterator_range(vertex_descriptor_map.right.equal_range(vertex))) {
				auto const& vertex_property =
				    graphs[i].get().get_vertex_property(vertex_descriptor);
				std::visit(
				    [&vps](auto const& vp) {
					    if constexpr (std::is_base_of_v<
					                      signal_flow::vertex::EntityOnChip,
					                      std::decay_t<decltype(vp)>>) {
						    vps.insert(vp.chip_coordinate);
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
				    " uses other chips than other graphs");
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

	// global data maps (each for one realtime_column)
	std::vector<signal_flow::OutputData> output_activation_maps(graphs.size());

	// build execution nodes
	for (auto const vertex :
	     boost::make_iterator_range(boost::vertices(execution_instance_graph))) {
		auto const execution_instance = execution_instance_map.left.at(vertex);
		// collect all chips used by execution instance
		std::set<DLSGlobal>& dls_globals = vps_per_execution_instance.at(vertex);
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

		// vector of configs to pass to execution_instance_node
		std::vector<std::reference_wrapper<lola::vx::v3::Chip const>> execution_node_configs;
		for (size_t i = 0; i < configs.size(); i++) {
			execution_node_configs.push_back(configs[i].get().at(execution_instance));
		}
		if (!playback_hooks.contains(execution_instance)) {
			playback_hooks[execution_instance] =
			    std::make_shared<signal_flow::ExecutionInstancePlaybackHooks>();
		}
		detail::ExecutionInstanceNode node_body(
		    output_activation_maps, inputs, graphs, execution_instance, dls_global,
		    execution_node_configs, executor.m_connections.at(dls_global),
		    executor.m_connection_state_storages.at(dls_global),
		    *(playback_hooks[execution_instance]));
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
	for (size_t i = 0; i < graphs.size(); i++) {
		signal_flow::ExecutionTimeInfo execution_time_info;
		execution_time_info.execution_duration = execution_duration;
		if (output_activation_maps[i].execution_time_info) {
			output_activation_maps[i].execution_time_info->merge(execution_time_info);
		} else {
			output_activation_maps[i].execution_time_info = execution_time_info;
		}
	}

	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
	LOG4CXX_INFO(
	    logger,
	    "run(): Executed graph in "
	        << hate::to_string(output_activation_maps[0].execution_time_info->execution_duration)
	        << ".");
	for (auto const& [dls, duration] :
	     output_activation_maps[0].execution_time_info->execution_duration_per_hardware) {
		LOG4CXX_INFO(
		    logger,
		    "run(): Chip at "
		        << dls << " spent " << hate::to_string(duration) << " in execution, which is "
		        << (static_cast<double>(duration.count()) /
		            static_cast<double>(
		                output_activation_maps[0].execution_time_info->execution_duration.count()) *
		            100.)
		        << " % of total graph execution time.");
	}

	return output_activation_maps;
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

std::vector<signal_flow::IODataList> run(
    JITGraphExecutor& executor,
    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataList const>> const& inputs,
    std::vector<std::reference_wrapper<JITGraphExecutor::ChipConfigs const>> const& configs,
    bool only_unconnected_output)
{
	JITGraphExecutor::PlaybackHooks empty;
	return run(executor, graphs, inputs, configs, empty, only_unconnected_output);
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

std::vector<signal_flow::IODataList> run(
    JITGraphExecutor& executor,
    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataList const>> const& inputs,
    std::vector<std::reference_wrapper<JITGraphExecutor::ChipConfigs const>> const& configs,
    JITGraphExecutor::PlaybackHooks& playback_hooks,
    bool only_unconnected_output)
{
	std::vector<signal_flow::InputData> input_maps(graphs.size());
	std::vector<std::reference_wrapper<signal_flow::InputData const>> input_map_refs;
	for (size_t i = 0; i < graphs.size(); i++) {
		input_maps[i] = inputs[i].get().to_input_map(graphs[i]);
		input_map_refs.push_back(input_maps[i]);
	}
	auto const output_maps = run(executor, graphs, input_map_refs, configs, playback_hooks);

	std::vector<signal_flow::IODataList> outputs(graphs.size());
	for (size_t i = 0; i < graphs.size(); i++) {
		outputs[i].from_output_map(output_maps[i], graphs[i], only_unconnected_output);
	}
	return outputs;
}

} // namespace grenade::vx::execution
