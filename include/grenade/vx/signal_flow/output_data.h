#pragma once
#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/signal_flow/data.h"
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

namespace grenade::vx::signal_flow GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

/**
 * Data map used for output data in graph execution.
 */
struct GENPYBIND(visible) OutputData : public Data
{
	/**
	 * Optional time information of performed execution to be filled by executor.
	 */
	std::optional<ExecutionTimeInfo> execution_time_info;

	/**
	 * Optional health information of performed execution to be filled by executor.
	 */
	std::optional<ExecutionHealthInfo> execution_health_info;

	typedef std::vector<std::map<
	    common::ExecutionInstanceID,
	    std::map<
	        std::string,
	        std::variant<
	            std::
	                map<halco::hicann_dls::vx::v3::HemisphereOnDLS, haldls::vx::v3::PPUMemoryBlock>,
	            lola::vx::v3::ExternalPPUMemoryBlock,
	            lola::vx::v3::ExternalPPUDRAMMemoryBlock>>>>
	    ReadPPUSymbols;

	/**
	 * Read PPU symbols corresponding to requested symbols in ExecutionInstanceHooks.
	 */
	ReadPPUSymbols read_ppu_symbols;

	typedef std::map<common::ExecutionInstanceID, lola::vx::v3::Chip> Chips;

	/**
	 * Pre-execution chip configuration including execution instance's static configuration.
	 */
	Chips pre_execution_chips;

	OutputData() SYMBOL_VISIBLE;

	OutputData(OutputData const&) = delete;

	OutputData(OutputData&& other) SYMBOL_VISIBLE;

	OutputData& operator=(OutputData&& other) SYMBOL_VISIBLE GENPYBIND(hidden);
	OutputData& operator=(OutputData const& other) = delete;

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(OutputData&& other) SYMBOL_VISIBLE GENPYBIND(hidden);

	/**
	 * Merge other map content into this one's.
	 * @param other Other map to merge into this instance
	 */
	void merge(OutputData& other) SYMBOL_VISIBLE;

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

} // namespace grenade::vx::signal_flow
