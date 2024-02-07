#pragma once
#include "grenade/vx/signal_flow/data.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/input_data.h"
#include "grenade/vx/signal_flow/output_data.h"
#include "grenade/vx/signal_flow/types.h"
#include "hate/visibility.h"
#include <list>
#include <vector>

namespace grenade::vx::signal_flow {

class Graph;

/**
 * Flat data list used for external data exchange in graph execution.
 * Entries are assumed to be in order with the occurence of ExternalInput vertices in the
 * corresponding graph (via boost::vertices()).
 */
struct IODataList
{
	typedef Data::Entry Entry;

	/** List of data entries. */
	std::list<Entry> data{};

	IODataList() = default;

	/**
	 * Convert to data list from data map with regard to the output vertices of the graph.
	 * @param map Map to convert
	 * @param graph Graph to use as reference for vertices
	 * @param only_unconnected Whether to convert only output vertices without out edges
	 */
	void from_output_map(
	    OutputData const& map,
	    signal_flow::Graph const& graph,
	    bool only_unconnected = true) SYMBOL_VISIBLE;

	/**
	 * Convert from data list to data map with regard to the output vertices of the graph.
	 * @param graph Graph to use as reference for vertices
	 * @param only_unconnected Whether to convert only output vertices without out edges
	 * @return Converted Map
	 */
	OutputData to_output_map(signal_flow::Graph const& graph, bool only_unconnected = true) const
	    SYMBOL_VISIBLE;

	/**
	 * Convert to data list from data map with regard to the input vertices of the graph.
	 * @param map Map to convert
	 * @param graph Graph to use as reference for vertices
	 */
	void from_input_map(InputData const& map, signal_flow::Graph const& graph) SYMBOL_VISIBLE;

	/**
	 * Convert from data list to data map with regard to the input vertices of the graph.
	 * @param graph Graph to use as reference for vertices
	 * @return Converted Map
	 */
	InputData to_input_map(signal_flow::Graph const& graph) const SYMBOL_VISIBLE;
};

} // namespace grenade::vx::signal_flow
