#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/common/time.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/signal_flow/data_snippet.h"
#include "grenade/vx/signal_flow/port.h"
#include "hate/visibility.h"
#include <map>
#include <vector>

namespace grenade::vx {
namespace signal_flow GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

/**
 * DataSnippet map used for input data in graph execution.
 */
struct GENPYBIND(visible) InputDataSnippet : public DataSnippet
{
	/**
	 * Runtime time-interval data.
	 * The runtime start time coincides with the spike events' and MADC recording start time.
	 * Event data is only recorded during the runtime.
	 * If the runtime data is empty it is ignored.
	 */
	std::vector<std::map<grenade::common::ExecutionInstanceID, common::Time>> runtime;

	InputDataSnippet() SYMBOL_VISIBLE;

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(InputDataSnippet&& other) SYMBOL_VISIBLE GENPYBIND(hidden);

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(InputDataSnippet& other) SYMBOL_VISIBLE;

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
