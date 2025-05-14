#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/signal_flow/data_snippet.h"
#include "grenade/vx/signal_flow/detail/graph.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/execution_health_info.h"
#include "grenade/vx/signal_flow/execution_time_info.h"
#include "grenade/vx/signal_flow/types.h"
#include "haldls/vx/v3/ppu.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"
#include "lola/vx/v3/ppu.h"
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <variant>

namespace grenade::vx {
namespace signal_flow GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

/**
 * DataSnippet map used for output data in graph execution.
 */
struct GENPYBIND(visible) OutputDataSnippet : public DataSnippet
{
	typedef std::map<grenade::common::ExecutionInstanceID, lola::vx::v3::Chip> Chips;

	/**
	 * Pre-execution chip configuration including execution instance's static configuration.
	 */
	Chips pre_execution_chips;

	OutputDataSnippet() SYMBOL_VISIBLE;

	OutputDataSnippet(DataSnippet&& other) SYMBOL_VISIBLE GENPYBIND(hidden);

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(OutputDataSnippet&& other) SYMBOL_VISIBLE GENPYBIND(hidden);

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(OutputDataSnippet& other) SYMBOL_VISIBLE;

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
