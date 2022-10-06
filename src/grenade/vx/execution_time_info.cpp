#include "grenade/vx/execution_time_info.h"

#include "hate/timer.h"
#include <ostream>

namespace grenade::vx {

void ExecutionTimeInfo::merge(ExecutionTimeInfo& other)
{
	merge(std::forward<ExecutionTimeInfo>(other));
}

void ExecutionTimeInfo::merge(ExecutionTimeInfo&& other)
{
	execution_duration = other.execution_duration;
	execution_duration_per_hardware.merge(other.execution_duration_per_hardware);
	realtime_duration_per_execution_instance.merge(other.realtime_duration_per_execution_instance);
}

std::ostream& operator<<(std::ostream& os, ExecutionTimeInfo const& info)
{
	os << "ExecutionTimeInfo(\n";
	os << "\texecution_duration: " << hate::to_string(info.execution_duration) << "\n";
	os << "\texecution_duration_per_hardware:\n";
	for (auto const& [dls, duration] : info.execution_duration_per_hardware) {
		os << "\t\t" << dls << ": " << hate::to_string(duration) << "\n";
	}
	os << ")";
	return os;
}

} // namespace grenade::vx
