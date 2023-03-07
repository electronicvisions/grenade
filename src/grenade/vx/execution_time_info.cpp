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
	execution_duration += other.execution_duration;

	execution_duration_per_hardware.merge(other.execution_duration_per_hardware);
	for (auto const& [dls, duration] : other.execution_duration_per_hardware) {
		execution_duration_per_hardware.at(dls) += duration;
	}
	other.execution_duration_per_hardware.clear();

	realtime_duration_per_execution_instance.merge(other.realtime_duration_per_execution_instance);
	for (auto const& [ei, duration] : other.realtime_duration_per_execution_instance) {
		realtime_duration_per_execution_instance.at(ei) += duration;
	}
	other.realtime_duration_per_execution_instance.clear();
}

std::ostream& operator<<(std::ostream& os, ExecutionTimeInfo const& info)
{
	os << "ExecutionTimeInfo(\n";
	os << "\texecution_duration: " << hate::to_string(info.execution_duration) << "\n";
	os << "\texecution_duration_per_hardware:\n";
	for (auto const& [dls, duration] : info.execution_duration_per_hardware) {
		os << "\t\t" << dls << ": " << hate::to_string(duration) << "\n";
	}
	os << "\trealtime_duration_per_execution_instance:\n";
	for (auto const& [instance, duration] : info.realtime_duration_per_execution_instance) {
		os << "\t\t" << instance << ": " << hate::to_string(duration) << "\n";
	}
	os << ")";
	return os;
}

} // namespace grenade::vx
