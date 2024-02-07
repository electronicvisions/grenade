#pragma once
#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/common/time.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/signal_flow/data.h"
#include "grenade/vx/signal_flow/port.h"
#include "hate/visibility.h"
#include <map>
#include <vector>

namespace grenade::vx::signal_flow GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

/**
 * Data map used for input data in graph execution.
 */
struct GENPYBIND(visible) InputData : public Data
{
	/**
	 * Runtime time-interval data.
	 * The runtime start time coincides with the spike events' and MADC recording start time.
	 * Event data is only recorded during the runtime.
	 * If the runtime data is empty it is ignored.
	 */
	std::vector<std::map<common::ExecutionInstanceID, common::Time>> runtime;

	/**
	 * Minimal waiting time between batch entries.
	 * For the case of multiple realtime columns (multiple snippets per batch entry),
	 * only the `inter_batch_entry_wait` of the last realtime column will be processed.
	 */
	std::map<common::ExecutionInstanceID, common::Time> inter_batch_entry_wait;

	InputData() SYMBOL_VISIBLE;

	InputData(InputData const&) = delete;

	InputData(InputData&& other) SYMBOL_VISIBLE;

	InputData& operator=(InputData&& other) SYMBOL_VISIBLE GENPYBIND(hidden);
	InputData& operator=(InputData const& other) = delete;

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(InputData&& other) SYMBOL_VISIBLE GENPYBIND(hidden);

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(InputData& other) SYMBOL_VISIBLE;

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

} // namespace grenade::vx::signal_flow
