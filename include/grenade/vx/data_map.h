#pragma once
#include <map>
#include <memory>
#include <mutex>
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
	std::map<Graph::vertex_descriptor, TimedSpikeSequence> spike_events;
	std::map<Graph::vertex_descriptor, TimedSpikeFromChipSequence> spike_event_output;

	DataMap() SYMBOL_VISIBLE;

	DataMap(DataMap const&) = delete;

	DataMap(DataMap&& other) SYMBOL_VISIBLE;

	DataMap& operator=(DataMap&& other) SYMBOL_VISIBLE;

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

private:
	/**
	 * Mutex guarding mutable operation merge() and clear().
	 * Mutable access to map content shall be guarded with this mutex.
	 */
	std::unique_ptr<std::mutex> mutex;
};

} // namespace grenade::vx
