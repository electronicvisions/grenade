#include "grenade/vx/execution/run.h"

#include "grenade/common/data.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/strong_component_invariant_vertex.h"
#include "grenade/common/topology.h"
#include "grenade/common/topology_rewrite/strong_component_invariant.h"
#include "grenade/vx/execution/backend/initialized_connection.h"
#include "grenade/vx/execution/detail/execution_instance_node.h"
#include "grenade/vx/network/abstract/execution_instance_global.h"
#include "grenade/vx/network/abstract/executor_global.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
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

grenade::common::OutputData run(
    JITGraphExecutor& executor,
    std::shared_ptr<grenade::common::Topology const> const& topology,
    grenade::common::InputData const& data,
    JITGraphExecutor::Hooks&& hooks)
{
	return std::move(run(executor,
	                     std::vector<std::shared_ptr<grenade::common::Topology const>>{topology},
	                     {data}, std::move(hooks))
	                     .at(0));
}

std::vector<grenade::common::OutputData> run(
    JITGraphExecutor& executor,
    std::vector<std::shared_ptr<grenade::common::Topology const>> const& topologies,
    std::vector<std::reference_wrapper<grenade::common::InputData const>> const& data,
    JITGraphExecutor::Hooks&& hooks)
{
	// ensure, that all vectors, which contain one element per realtime snippet are of the same size
	if (topologies.size() != data.size()) {
		throw std::logic_error("Arguments 'topologies' and 'data' must be of the same size");
	}
	// ensure, that all vectors contain at least one element, otherwise the experiment is not
	// existent
	if (topologies.size() < 1) {
		throw std::logic_error(
		    "Arguments 'topologies' and 'data' must contain at least one element");
	}

	using namespace halco::hicann_dls::vx::v3;
	hate::Timer const timer;

	// construct linked topology per topology to execute, which contains vertices per strong
	// component invariant and their dependencies as edges
	std::vector<std::shared_ptr<grenade::common::LinkedTopology>> execution_instance_topologies;
	for (auto const& graph : topologies) {
		execution_instance_topologies.emplace_back(
		    std::make_shared<grenade::common::LinkedTopology>(graph));
	}
	for (auto& execution_instance_topology : execution_instance_topologies) {
		grenade::common::StrongComponentInvariantRewrite rewrite(execution_instance_topology);
		rewrite();
	}

	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::vector<grenade::common::VertexOnTopology>>
	    vertices_per_execution_instance;

	for (size_t i = 0; i < execution_instance_topologies.size(); ++i) {
		for (auto const& vertex_descriptor : execution_instance_topologies.at(i)->vertices()) {
			auto const& vertex =
			    dynamic_cast<grenade::common::StrongComponentInvariantVertex const&>(
			        execution_instance_topologies.at(i)->get(vertex_descriptor));
			auto const strong_component_invariant = vertex.get_strong_component_invariant();
			assert(strong_component_invariant);
			vertices_per_execution_instance
			    [dynamic_cast<grenade::common::PartitionedVertex::StrongComponentInvariant const&>(
			         *strong_component_invariant)
			         .execution_instance_on_executor.value()]
			        .push_back(vertex_descriptor);
		}
	}

	// check, that all topologies have equal execution instance topologies and use the same
	// chips
	for (size_t i = 1; i < topologies.size(); i++) {
		if (static_cast<grenade::common::Topology const&>(*execution_instance_topologies.at(i)) !=
		    static_cast<grenade::common::Topology const&>(*execution_instance_topologies.at(0))) {
			std::stringstream ss;
			ss << "Graph corresponding to configuration " << i
			   << " has differing execution instance topology from other topologies:\n";
			ss << i << ": " << *execution_instance_topologies.at(i) << "\n";
			ss << "others: " << *execution_instance_topologies.at(0);
			throw std::runtime_error(ss.str());
		}
	}

	// execution graph
	tbb::flow::graph execution_graph;
	// start trigger node
	tbb::flow::broadcast_node<tbb::flow::continue_msg> start(execution_graph);

	// execution nodes
	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    tbb::flow::continue_node<tbb::flow::continue_msg>>
	    nodes;

	// global data maps (each for one realtime_column)
	std::vector<grenade::common::OutputData> results(topologies.size());
	std::mutex results_mutex;

	// build execution nodes
	for (auto const& [execution_instance_on_executor, execution_instance_vertex_descriptors] :
	     vertices_per_execution_instance) {
		if (!hooks.contains(execution_instance_on_executor)) {
			hooks[execution_instance_on_executor] = std::make_shared<ExecutionInstanceHooks>();
			for (auto const& chip_on_connection :
			     executor.m_connections.at(execution_instance_on_executor.connection_on_executor)
			         .get_chips_on_connection()) {
				hooks.at(execution_instance_on_executor)->chips[chip_on_connection] = {};
			}
		} else {
			for (auto const& chip_on_connection :
			     executor.m_connections.at(execution_instance_on_executor.connection_on_executor)
			         .get_chips_on_connection()) {
				if (!hooks.at(execution_instance_on_executor)->chips.contains(chip_on_connection)) {
					hooks.at(execution_instance_on_executor)->chips[chip_on_connection] = {};
				}
			}
		}
		detail::ExecutionInstanceNode node_body(
		    results, results_mutex, execution_instance_topologies, execution_instance_on_executor,
		    data, executor.m_connections.at(execution_instance_on_executor.connection_on_executor),
		    *(hooks.at(execution_instance_on_executor)), execution_instance_vertex_descriptors);
		nodes.insert(std::make_pair(
		    execution_instance_on_executor,
		    tbb::flow::continue_node<tbb::flow::continue_msg>(execution_graph, node_body)));
	}

	// connect execution nodes
	for (auto const vertex_descriptor : execution_instance_topologies.at(0)->vertices()) {
		if (execution_instance_topologies.at(0)->in_degree(vertex_descriptor) == 0) {
			auto const& vertex =
			    dynamic_cast<grenade::common::StrongComponentInvariantVertex const&>(
			        execution_instance_topologies.at(0)->get(vertex_descriptor));
			auto const strong_component_invariant = vertex.get_strong_component_invariant();
			assert(strong_component_invariant);
			tbb::flow::make_edge(
			    start,
			    nodes.at(dynamic_cast<
			                 grenade::common::PartitionedVertex::StrongComponentInvariant const&>(
			                 *strong_component_invariant)
			                 .execution_instance_on_executor.value()));
		}
		for (auto const out_edge :
		     execution_instance_topologies.at(0)->out_edges(vertex_descriptor)) {
			auto const& vertex =
			    dynamic_cast<grenade::common::StrongComponentInvariantVertex const&>(
			        execution_instance_topologies.at(0)->get(vertex_descriptor));
			auto const strong_component_invariant = vertex.get_strong_component_invariant();
			assert(strong_component_invariant);

			auto const target_descriptor = execution_instance_topologies.at(0)->target(out_edge);
			auto const& target =
			    dynamic_cast<grenade::common::StrongComponentInvariantVertex const&>(
			        execution_instance_topologies.at(0)->get(target_descriptor));
			auto const target_strong_component_invariant = target.get_strong_component_invariant();
			assert(strong_component_invariant);
			tbb::flow::make_edge(
			    nodes.at(dynamic_cast<
			                 grenade::common::PartitionedVertex::StrongComponentInvariant const&>(
			                 *strong_component_invariant)
			                 .execution_instance_on_executor.value()),
			    nodes.at(dynamic_cast<
			                 grenade::common::PartitionedVertex::StrongComponentInvariant const&>(
			                 *target_strong_component_invariant)
			                 .execution_instance_on_executor.value()));
		}
	}

	// trigger execution and wait for completion
	start.try_put(tbb::flow::continue_msg());
	execution_graph.wait_for_all();
	std::chrono::nanoseconds execution_duration(timer.get_ns());

	for (auto& result : results) {
		network::abstract::ExecutorGlobal executor_global;
		executor_global.execution_duration = execution_duration;
		result.set_executor(executor_global);
	}
	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
	LOG4CXX_INFO(logger, "run(): Executed graph in " << hate::to_string(execution_duration) << ".");
	for (auto const& [execution_instance_on_executor, execution_instance] :
	     results[0].execution_instances) {
		auto const& execution_instance_global =
		    dynamic_cast<network::abstract::ExecutionInstanceGlobal const&>(execution_instance);
		for (auto const& [chip_on_connection, duration] :
		     execution_instance_global.device_usage_duration) {
			LOG4CXX_INFO(
			    logger, "run(): Chip at ("
			                << execution_instance_on_executor << ", " << chip_on_connection
			                << ") spent " << hate::to_string(duration) << " in execution, which is "
			                << (static_cast<double>(duration.count()) /
			                    static_cast<double>(execution_duration.count()) * 100.)
			                << " % of total graph execution time.");
		}
		if (execution_instance_global.execution_health_info) {
			LOG4CXX_TRACE(
			    logger, "run(): " << *execution_instance_global.execution_health_info << ".");
		}
	}

	return results;
}

} // namespace grenade::vx::execution
