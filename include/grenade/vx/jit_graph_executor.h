#pragma once
#include <map>

#include "hate/visibility.h"
#include "hxcomm/vx/connection_variant.h"

namespace halco::hicann_dls::vx {
struct DLSGlobal;
struct HemishereOnDLS;
} // namespace halco::hicann_dls::vx

namespace grenade::vx {

class Graph;
class DataMap;
class ChipConfig;

/**
 * Just-in-time graph executor.
 * No partial playback stream is pre-generated and nothing is reused for another run with e.g.
 * different input. This especially allows implementation without promise support, since before
 * processing each new chip instance, all necessary results are already available by value.
 */
class JITGraphExecutor
{
public:
	/** List of executors. */
	typedef std::map<halco::hicann_dls::vx::DLSGlobal, hxcomm::vx::ConnectionVariant&> ExecutorMap;

	typedef std::map<halco::hicann_dls::vx::DLSGlobal, ChipConfig> ConfigMap;

	/**
	 * Run the specified graph with specified inputs using the specified executor collection.
	 * @param graph Graph to execute
	 * @param input_list List of input values to use
	 * @param executor_map Map of executors tied to a specific chip instance
	 * @param config_map Map of static configuration tied to a specific chip instance
	 */
	static DataMap run(
	    Graph const& graph,
	    DataMap const& input_list,
	    ExecutorMap const& executor_map,
	    ConfigMap const& config_map) SYMBOL_VISIBLE;

private:
	/**
	 * Check whether the given graph can be executed on the given map of executors.
	 * @param graph Graph instance
	 * @param executor_map Map of executors over DLSGlobal identifier
	 */
	static bool is_executable_on(Graph const& graph, ExecutorMap const& executor_map);

	/**
	 * Check whether the given input list embodies all input values demanded by the given graph.
	 * @param input_list Input list to check
	 * @param graph Graph to check against
	 */
	static bool is_complete_input_list_for(DataMap const& input_list, Graph const& graph);

	/**
	 * Check that graph can be executed given the input list and the map of executors.
	 * This function combines `is_executable_on`, `is_complete_input_list_for` and
	 * `has_dangling_inputs`.
	 * @param graph Graph to check
	 * @param input_list Input list to check
	 * @param executor_map Executor map to check
	 */
	static void check(
	    Graph const& graph, DataMap const& input_list, ExecutorMap const& executor_map);
};

} // namespace grenade::vx
