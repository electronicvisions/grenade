#pragma once
#include <map>
#include "grenade/vx/event.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/types.h"
#include "hate/visibility.h"

namespace grenade::vx {

/**
 * Data map used for external data exchange in graph execution.
 * For each type of data a separate member allows access.
 */
struct DataMap
{
	std::map<Graph::vertex_descriptor, std::vector<Int8>> int8;
	std::map<Graph::vertex_descriptor, TimedSpikeEventSequence> spike_events;

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(DataMap&& other) SYMBOL_VISIBLE;

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(DataMap& other) SYMBOL_VISIBLE;

	/**
	 * Clear content of map.
	 */
	void clear() SYMBOL_VISIBLE;
};

} // namespace grenade::vx
