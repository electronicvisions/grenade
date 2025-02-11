#pragma once
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/output_data.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/detail/generator/ppu.h"
#include "grenade/vx/signal_flow/execution_health_info.h"
#include "haldls/vx/v3/ppu.h"
#include "lola/vx/v3/ppu.h"
#include <chrono>
#include <variant>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include <pybind11/chrono.h>
#endif

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Global results for BrainScaleS-2 backend.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) ExecutionInstanceGlobal
    : grenade::common::OutputData::ExecutionInstance
{
	/**
	 * Time spent in execution on hardware.
	 * This is the accumulated time each connection spends in execution state of executing playback
	 * program instruction streams, that is from encoding and sending instructions to receiving all
	 * responses and decoding them up to the halt response.
	 */
	std::map<common::ChipOnConnection, std::chrono::nanoseconds> device_usage_duration{};

	/**
	 * Time spent in realtime section on hardware.
	 * This is the accumulated time for each execution instance of the interval [0, runtime) for
	 * each batch entry. It is equivalent to the accumulated duration of the intervals during which
	 * event recording is enabled for each batch entry.
	 */
	std::map<common::ChipOnConnection, std::chrono::nanoseconds> realtime_duration{};

	/**
	 * Health information of performed execution to be filled by executor.
	 */
	std::optional<signal_flow::ExecutionHealthInfo::ExecutionInstance> execution_health_info;

	typedef std::vector<std::map<
	    common::ChipOnConnection,
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

	/**
	 * Pre-execution chip configuration including execution instance's static configuration.
	 */
	std::map<common::ChipOnConnection, lola::vx::v3::Chip> pre_execution_chips;

	virtual std::unique_ptr<grenade::common::OutputData::ExecutionInstance> copy() const override;
	virtual std::unique_ptr<grenade::common::OutputData::ExecutionInstance> move() override;

protected:
	virtual bool is_equal_to(
	    grenade::common::OutputData::ExecutionInstance const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
