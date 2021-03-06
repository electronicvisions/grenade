#pragma once
#include <map>
#include <unordered_map>

#include "grenade/vx/execution_instance.h"
#include "grenade/vx/execution_instance_playback_hooks.h"
#include "halco/hicann-dls/vx/v2/chip.h"
#include "hate/visibility.h"

namespace grenade::vx {

namespace backend {
struct Connection;
} // namespace backend

class Graph;
class IODataMap;
class IODataList;
class ChipConfig;
class ExecutionInstancePlaybackHooks;

/**
 * Just-in-time graph executor.
 * No partial playback stream is pre-generated and nothing is reused for another run with e.g.
 * different input. This especially allows implementation without promise support, since before
 * processing each new chip instance, all necessary results are already available by value.
 */
class JITGraphExecutor
{
public:
	/** Map of connections. */
	typedef std::map<halco::hicann_dls::vx::v2::DLSGlobal, backend::Connection&> Connections;

	typedef std::map<halco::hicann_dls::vx::v2::DLSGlobal, ChipConfig> ChipConfigs;

	typedef std::unordered_map<coordinate::ExecutionInstance, ExecutionInstancePlaybackHooks>
	    PlaybackHooks;

	/**
	 * Run the specified graph with specified inputs using the specified connection collection.
	 * @param graph Graph to execute
	 * @param input_list List of input values to use
	 * @param connections Map of connections tied to a specific chip instance
	 * @param chip_configs Map of static configuration tied to a specific chip instance
	 */
	static IODataMap run(
	    Graph const& graph,
	    IODataMap const& input_list,
	    Connections const& connections,
	    ChipConfigs const& chip_configs) SYMBOL_VISIBLE;

	/**
	 * Run the specified graph with specified inputs using the specified connection collection.
	 * @param graph Graph to execute
	 * @param input_list List of input values to use
	 * @param connections Map of connections tied to a specific chip instance
	 * @param chip_configs Map of static configuration tied to a specific chip instance
	 * @param playback_hooks Map of playback sequence collections to be inserted at specified
	 * execution instances
	 */
	static IODataMap run(
	    Graph const& graph,
	    IODataMap const& input_list,
	    Connections const& connections,
	    ChipConfigs const& chip_configs,
	    PlaybackHooks& playback_hooks) SYMBOL_VISIBLE;

	/**
	 * Run the specified graph with specified inputs using the specified connection collection.
	 * @param graph Graph to execute
	 * @param input_list List of input values to use
	 * @param connections Map of connections tied to a specific chip instance
	 * @param chip_configs Map of static configuration tied to a specific chip instance
	 * @param only_unconnected_output Whether to return only values to output vertices without out
	 * edges
	 */
	static IODataList run(
	    Graph const& graph,
	    IODataList const& input_list,
	    Connections const& connections,
	    ChipConfigs const& chip_configs,
	    bool only_unconnected_output = true) SYMBOL_VISIBLE;

	/**
	 * Run the specified graph with specified inputs using the specified connection collection.
	 * @param graph Graph to execute
	 * @param input_list List of input values to use
	 * @param connections Map of connections tied to a specific chip instance
	 * @param chip_configs Map of static configuration tied to a specific chip instance
	 * @param playback_hooks Map of playback sequence collections to be inserted at specified
	 * execution instances
	 * @param only_unconnected_output Whether to return only values to output vertices without out
	 * edges
	 */
	static IODataList run(
	    Graph const& graph,
	    IODataList const& input_list,
	    Connections const& connections,
	    ChipConfigs const& chip_configs,
	    PlaybackHooks& playback_hooks,
	    bool only_unconnected_output = true) SYMBOL_VISIBLE;

private:
	/**
	 * Check whether the given graph can be executed on the given map of connections.
	 * @param graph Graph instance
	 * @param connections Map of connections
	 */
	static bool is_executable_on(Graph const& graph, Connections const& connections);

	/**
	 * Check that graph can be executed given the input list and the map of connections.
	 * This function combines `is_executable_on` and
	 * `has_dangling_inputs`.
	 * @param graph Graph to check
	 * @param connections Connections to check
	 */
	static void check(Graph const& graph, Connections const& connections);
};

} // namespace grenade::vx
