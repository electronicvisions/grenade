#pragma once
#include "grenade/vx/event.h"
#include "grenade/vx/graph_representation.h"
#include "grenade/vx/types.h"
#include "hate/visibility.h"
#include <map>
#include <memory>
#include <mutex>

namespace grenade::vx {

/**
 * Data map used for external data exchange in graph execution.
 * For each type of data a separate member allows access.
 */
struct IODataMap
{
	/**
	 * Data is connected to specified vertex descriptors.
	 * Batch-support is enabled by storing batch-size many data elements aside each-other.
	 * @tparam Data Batch-entry data type
	 */
	template <typename Data>
	using DataTypeMap = std::map<detail::vertex_descriptor, std::vector<Data>>;

	/** UInt32 data. */
	DataTypeMap<std::vector<UInt32>> uint32;
	/** UInt5 data. */
	DataTypeMap<std::vector<UInt5>> uint5;
	/** Int8 data. */
	DataTypeMap<std::vector<Int8>> int8;
	/** Spike input events. */
	DataTypeMap<TimedSpikeSequence> spike_events;
	/** Spike output events. */
	DataTypeMap<TimedSpikeFromChipSequence> spike_event_output;

	IODataMap() SYMBOL_VISIBLE;

	IODataMap(IODataMap const&) = delete;

	IODataMap(IODataMap&& other) SYMBOL_VISIBLE;

	IODataMap& operator=(IODataMap&& other) SYMBOL_VISIBLE;

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(IODataMap&& other) SYMBOL_VISIBLE;

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(IODataMap& other) SYMBOL_VISIBLE;

	/**
	 * Clear content of map.
	 */
	void clear() SYMBOL_VISIBLE;

	/**
	 * Get whether the map does not contain any elements.
	 * @return Boolean value
	 */
	bool empty() const SYMBOL_VISIBLE;

	/**
	 * Get number of elements in each batch of data.
	 * @return Number of elements in batch
	 */
	size_t batch_size() const SYMBOL_VISIBLE;

	/**
	 * Check that all map entries feature the same batch_size value.
	 * @return Boolean value
	 */
	bool valid() const SYMBOL_VISIBLE;

private:
	/**
	 * Mutex guarding mutable operation merge() and clear().
	 * Mutable access to map content shall be guarded with this mutex.
	 */
	std::unique_ptr<std::mutex> mutex;
};


/**
 * Data map of constant references to data used for external data input in graph execution.
 * For each type of data a separate member allows access.
 */
struct ConstantReferenceIODataMap
{
	/**
	 * Data is connected to specified vertex descriptors.
	 * Batch-support is enabled by storing batch-size many data elements aside each-other.
	 * @tparam Data Batch-entry data type
	 */
	template <typename Data>
	using DataTypeMap = std::map<detail::vertex_descriptor, std::vector<Data> const&>;

	/** UInt32 data. */
	DataTypeMap<std::vector<UInt32>> uint32;
	/** UInt5 data. */
	DataTypeMap<std::vector<UInt5>> uint5;
	/** Int8 data. */
	DataTypeMap<std::vector<Int8>> int8;
	/** Spike input events. */
	DataTypeMap<TimedSpikeSequence> spike_events;
	/** Spike output events. */
	DataTypeMap<TimedSpikeFromChipSequence> spike_event_output;

	ConstantReferenceIODataMap() SYMBOL_VISIBLE;

	/**
	 * Clear content of map.
	 */
	void clear() SYMBOL_VISIBLE;
};

} // namespace grenade::vx
