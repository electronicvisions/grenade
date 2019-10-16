#include "grenade/vx/jit_graph_executor.h"

#include <algorithm>
#include <future>
#include <map>
#include <set>
#include <vector>
#include <boost/range/adaptor/map.hpp>
#include <log4cxx/logger.h>

#include "grenade/vx/config.h"
#include "grenade/vx/data_map.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/execution_instance_builder.h"
#include "grenade/vx/launch_policy.h"
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

	check(graph, input_list, executor_map);

	// generate map of map of vertex descriptors with temporal index and dls global as keys
	auto const ordered_vertices = graph.get_ordered_vertices();

	std::map<halco::hicann_dls::vx::HemisphereGlobal, ExecutionInstanceBuilder>
	    execution_instance_builder_map;
	DataMap output_activation_map;
	// unroll time steps sequentially
	for (auto const& [temporal_index, dls_global_vertices_map] : ordered_vertices) {
		// unroll physical chips sequentially
		std::map<halco::hicann_dls::vx::DLSGlobal, std::future<DataMap>> time_step_results;
		for (auto const& [dls_global, vertices] : dls_global_vertices_map) {
			time_step_results[dls_global] = std::async(detail::launch_policy, [&]() {
				auto const local_vertex_count = [vertices]() {
					return std::accumulate(
					    vertices.begin(), vertices.end(), 0,
					    [](auto const& a, auto const& b) { return a + b.size(); });
				};
				auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
				LOG4CXX_DEBUG(
				    logger, "run(): Executing " << local_vertex_count() << " vertices for "
				                                << temporal_index << " on " << dls_global);

				for (auto const hemisphere : iter_all<HemisphereOnDLS>()) {
					HemisphereGlobal hemisphere_core_global(hemisphere, dls_global);
					if (!execution_instance_builder_map.count(hemisphere_core_global)) {
						execution_instance_builder_map.insert(std::make_pair(
						    hemisphere_core_global,
						    ExecutionInstanceBuilder(
						        graph, input_list, output_activation_map, hemisphere,
						        config_map.at(dls_global).hemispheres[hemisphere])));
					}
				}
				auto& top_builder = execution_instance_builder_map.at(
				    HemisphereGlobal(HemisphereOnDLS(0), dls_global));
				auto& bot_builder = execution_instance_builder_map.at(
				    HemisphereGlobal(HemisphereOnDLS(1), dls_global));
				return run_chip_instance(
				    top_builder, bot_builder, executor_map.at(dls_global), vertices);
			});
		}
		for (auto& result : time_step_results) {
			output_activation_map.merge(result.second.get());
		}
	}

	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
	LOG4CXX_TRACE(logger, "run(): Executed graph in " << timer.print() << ".");

	return output_activation_map;
}

DataMap JITGraphExecutor::run_chip_instance(
    ExecutionInstanceBuilder& top_builder,
    ExecutionInstanceBuilder& bot_builder,
    hxcomm::vx::ConnectionVariant& connection,
    halco::common::typed_array<
        std::vector<Graph::vertex_descriptor>,
        halco::hicann_dls::vx::HemisphereOnDLS> const& vertices)
{
	using namespace stadls::vx;
	using namespace halco::common;
	using namespace halco::hicann_dls::vx;
	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");

	DataMap output_activation_map;

	hate::Timer const build_timer;
	// visit vertices
	halco::common::typed_array<std::future<PlaybackProgramBuilder>, HemisphereOnDLS> results;
	auto const process = [vertices, &top_builder, &bot_builder](auto const core) {
		auto& builder = core ? bot_builder : top_builder;
		for (auto const vertex : vertices[core]) {
			builder.process(vertex);
		}
		return builder.generate();
	};
	for (auto const core : iter_all<HemisphereOnDLS>()) {
		results[core] = std::async(detail::launch_policy, process, core);
	}

	// build PlaybackProgram
	auto builder = results[HemisphereOnDLS(0)].get();
	auto ppb_bot = results[HemisphereOnDLS(1)].get();
	builder.merge_back(ppb_bot);
	LOG4CXX_TRACE(
	    logger, "run_chip_instance(): Built PlaybackProgram in " << build_timer.print() << ".");

	// execute
	hate::Timer const exec_timer;
	if (!builder.empty()) {
		auto program = builder.done();
		stadls::vx::run(connection, program);
	}
	LOG4CXX_TRACE(
	    logger,
	    "run_chip_instance(): Executed built PlaybackProgram in " << exec_timer.print() << ".");

	// extract output activations and place in map
	hate::Timer const post_timer;
	halco::common::typed_array<std::future<DataMap>, HemisphereOnDLS> activation_maps;
	for (auto const core : iter_all<HemisphereOnDLS>()) {
		auto process_result = [&bot_builder, &top_builder](auto const core) -> DataMap {
			auto& builder = core ? bot_builder : top_builder;
			return builder.post_process();
		};
		activation_maps[core] = std::async(detail::launch_policy, process_result, core);
	}
	for (auto const core : iter_all<HemisphereOnDLS>()) {
		output_activation_map.merge(activation_maps[core].get());
	}
	LOG4CXX_TRACE(logger, "run_chip_instance(): Evaluated in " << post_timer.print() << ".");

	return output_activation_map;
}


bool JITGraphExecutor::is_executable_on(Graph const& graph, ExecutorMap const& executor_map)
{
	auto const executor_dls_globals = boost::adaptors::keys(executor_map);
	auto const& execution_instance_map =
	    boost::get(Graph::ExecutionInstancePropertyTag(), graph.get_graph());

	auto const vertices = boost::make_iterator_range(boost::vertices(graph.get_graph()));
	return std::none_of(vertices.begin(), vertices.end(), [&](auto const vertex) {
		return std::find(
		           executor_dls_globals.begin(), executor_dls_globals.end(),
		           execution_instance_map[vertex].toHemisphereGlobal().toDLSGlobal()) ==
		       executor_dls_globals.end();
	});
}

bool JITGraphExecutor::is_complete_input_list_for(DataMap const& input_list, Graph const& graph)
{
	auto const& vertex_map = boost::get(Graph::VertexPropertyTag(), graph.get_graph());
	auto const vertices = boost::make_iterator_range(boost::vertices(graph.get_graph()));
	return std::none_of(vertices.begin(), vertices.end(), [&](auto const vertex) {
		if (std::holds_alternative<vertex::ExternalInput>(vertex_map[vertex])) {
			auto const& input_vertex = std::get<vertex::ExternalInput>(vertex_map[vertex]);
			if (input_vertex.output().type == ConnectionType::DataOutputUInt5) {
				if (input_list.uint5.find(vertex) == input_list.uint5.end()) {
					return true;
				} else if (input_list.uint5.at(vertex).size() != input_vertex.output().size) {
					return true;
				}
			} else if (input_vertex.output().type == ConnectionType::DataOutputInt8) {
				if (input_list.int8.find(vertex) == input_list.int8.end()) {
					return true;
				} else if (input_list.int8.at(vertex).size() != input_vertex.output().size) {
					return true;
				}
			} else {
				throw std::runtime_error("ExternalInput output type not supported.");
			}
		}
		return false;
	});
}

void JITGraphExecutor::check(
    Graph const& graph, DataMap const& input_list, ExecutorMap const& executor_map)
{
	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
	hate::Timer const timer;

	// check all DLSGlobal physical chips used in the graph are present in the provided executor map
	if (!is_executable_on(graph, executor_map)) {
		throw std::runtime_error("Graph requests executors not provided.");
	}

	// check that input list provides all requested input for graph
	if (!is_complete_input_list_for(input_list, graph)) {
		throw std::runtime_error("Graph requests unprovided input.");
	}

	LOG4CXX_TRACE(
	    logger,
	    "check(): Checked fit of graph, input list and executors in " << timer.print() << ".");
}

} // namespace grenade::vx
