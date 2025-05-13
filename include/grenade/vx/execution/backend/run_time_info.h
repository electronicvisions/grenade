#pragma once
#include "grenade/vx/common/chip_on_connection.h"
#include <chrono>
#include <map>

namespace grenade::vx::execution::backend {

/**
 * Time info of run of playback program on stateful connection.
 */
struct RunTimeInfo
{
	/**
	 * Accumulated execution duration of all stadls playback program executions per chip on
	 * connection.
	 */
	std::map<common::ChipOnConnection, std::chrono::nanoseconds> execution_duration;
};

} // namespace grenade::vx::execution::backend
