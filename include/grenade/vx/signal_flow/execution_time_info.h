#pragma once
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <chrono>
#include <iosfwd>
#include <map>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include <pybind11/chrono.h>
#endif

namespace grenade::vx {
namespace signal_flow GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

struct GENPYBIND(visible) ExecutionTimeInfo
{
	/**
	 * Time spent in execution on hardware.
	 * This is the accumulated time each connection spends in execution state of executing playback
	 * program instruction streams, that is from encoding and sending instructions to receiving all
	 * responses and decoding them up to the halt response.
	 */
	std::map<
	    grenade::common::ConnectionOnExecutor,
	    std::map<common::ChipOnConnection, std::chrono::nanoseconds>>
	    execution_duration_per_hardware{};

	/**
	 * Time spent in realtime section on hardware.
	 * This is the accumulated time for each execution instance of the interval [0, runtime) for
	 * each batch entry. It is equivalent to the accumulated duration of the intervals during which
	 * event recording is enabled for each batch entry.
	 */
	std::map<
	    grenade::common::ExecutionInstanceID,
	    std::map<common::ChipOnConnection, std::chrono::nanoseconds>>
	    realtime_duration_per_execution_instance{};

	/**
	 * Total duration of execution.
	 * This includes graph traversal, compilation of playback programs and post-processing of
	 * result. data.
	 */
	std::chrono::nanoseconds execution_duration{0};

	/**
	 * Merge other execution time info.
	 * This merges all map-like structures and accumulates the others as well as already present map
	 * entries.
	 * @param other Other execution time info to merge
	 */
	void merge(ExecutionTimeInfo& other) SYMBOL_VISIBLE;

	/**
	 * Merge other execution time info.
	 * This merges all map-like structures and accumulates the others as well as already present map
	 * entries.
	 * @param other Other execution time info to merge
	 */
	void merge(ExecutionTimeInfo&& other) SYMBOL_VISIBLE GENPYBIND(hidden);

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, ExecutionTimeInfo const& data) SYMBOL_VISIBLE;
};

} // namespace signal_flow
} // namespace grenade::vx
