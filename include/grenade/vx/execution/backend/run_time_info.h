#pragma once
#include <chrono>

namespace grenade::vx::execution::backend {

/**
 * Time info of run of playback program on stateful connection.
 */
struct RunTimeInfo
{
	/**
	 * Accumulated execution duration of all stadls playback program executions.
	 */
	std::chrono::nanoseconds execution_duration;
};

} // namespace grenade::vx::execution::backend
