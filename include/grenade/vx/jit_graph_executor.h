#pragma once
#include <map>

#include "halco/hicann-dls/vx/v2/chip.h"
#include "hate/visibility.h"
#include "hxcomm/vx/connection_variant.h"

namespace grenade::vx {

class Graph;
class IODataMap;
class IODataList;
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
	/** Map of connections. */
	typedef std::map<halco::hicann_dls::vx::v2::DLSGlobal, hxcomm::vx::ConnectionVariant&>
	    Connections;

	typedef std::map<halco::hicann_dls::vx::v2::DLSGlobal, ChipConfig> ChipConfigs;

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
	 * @param only_unconnected_output Whether to return only values to output vertices without out
	 * edges
	 */
	static IODataList run(
	    Graph const& graph,
	    IODataList const& input_list,
	    Connections const& connections,
	    ChipConfigs const& chip_configs,
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
