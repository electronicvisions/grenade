#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/common/time.h"
#include "grenade/vx/common/timed_data.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/signal_flow/detail/graph.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/port.h"
#include "grenade/vx/signal_flow/types.h"
#include "hate/visibility.h"
#include <map>
#include <memory>
#include <mutex>
#include <variant>

namespace grenade::vx {
namespace signal_flow GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

/**
 * Data map used for external data exchange in graph execution.
 * For each type of data a separate member allows access.
 */
struct GENPYBIND(visible) Data
{
	/**
	 * Data entry for a vertex.
	 */
	typedef std::variant<
	    std::vector<common::TimedDataSequence<std::vector<UInt32>>>,
	    std::vector<common::TimedDataSequence<std::vector<UInt5>>>,
	    std::vector<common::TimedDataSequence<std::vector<Int8>>>,
	    std::vector<TimedSpikeToChipSequence>,
	    std::vector<TimedSpikeFromChipSequence>,
	    std::vector<TimedMADCSampleFromChipSequence>>
	    Entry;

	/**
	 * Get whether the data held in the entry match the port shape and type information.
	 * @param entry Entry to check
	 * @param port Port to check
	 * @return  Boolean value
	 */
	static bool is_match(Entry const& entry, signal_flow::Port const& port) SYMBOL_VISIBLE;

	/**
	 * Data is connected to specified vertex descriptors.
	 * Batch-support is enabled by storing batch-size many data elements aside each-other.
	 * Data is stored as shared_ptr to allow for inexpensive shallow copy of data for multiple
	 * vertices in the map.
	 */
	std::map<signal_flow::detail::vertex_descriptor, Entry> data GENPYBIND(hidden);

	Data() SYMBOL_VISIBLE;

	Data(Data const&) = delete;

	Data(Data&& other) SYMBOL_VISIBLE;

	Data& operator=(Data&& other) SYMBOL_VISIBLE GENPYBIND(hidden);
	Data& operator=(Data const& other) = delete;

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(Data&& other) SYMBOL_VISIBLE GENPYBIND(hidden);

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(Data& other) SYMBOL_VISIBLE;

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
};

} // namespace signal_flow
} // namespace grenade::vx
