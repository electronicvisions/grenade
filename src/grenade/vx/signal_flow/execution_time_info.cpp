#include "grenade/vx/signal_flow/execution_time_info.h"

#include "hate/timer.h"
#include <chrono>
#include <ostream>

namespace grenade::vx::signal_flow {

void ExecutionTimeInfo::merge(ExecutionTimeInfo& other)
{
	merge(std::forward<ExecutionTimeInfo>(other));
}

void ExecutionTimeInfo::merge(ExecutionTimeInfo&& other)
{
	execution_duration += other.execution_duration;

	execution_duration_per_hardware.merge(other.execution_duration_per_hardware);
	for (auto const& [connection_on_executor, duration_per_chip] :
	     other.execution_duration_per_hardware) {
		for (auto const& [chip_on_connection, duration] : duration_per_chip) {
			execution_duration_per_hardware.at(connection_on_executor)[chip_on_connection] +=
			    duration;
		}
	}
	other.execution_duration_per_hardware.clear();

	realtime_duration_per_execution_instance.merge(other.realtime_duration_per_execution_instance);
	for (auto const& [ei, duration_per_chip] : other.realtime_duration_per_execution_instance) {
		for (auto const& [chip_on_connection, duration] : duration_per_chip) {
			realtime_duration_per_execution_instance.at(ei)[chip_on_connection] += duration;
		}
		other.realtime_duration_per_execution_instance.clear();
	}
}

std::ostream& operator<<(std::ostream& os, ExecutionTimeInfo const& info)
{
	os << "ExecutionTimeInfo(\n";
	os << "\texecution_duration: " << hate::to_string(info.execution_duration) << "\n";
	os << "\texecution_duration_per_hardware:\n";
	for (auto const& [connection_on_executor, duration_per_chip] :
	     info.execution_duration_per_hardware) {
		for (auto const& [chip_on_connection, duration] : duration_per_chip) {
			os << "\t\t" << connection_on_executor << ", " << chip_on_connection << ": "
			   << hate::to_string(duration) << "\n";
		}
	}
	os << "\trealtime_duration_per_execution_instance:\n";
	for (auto const& [instance, duration_per_chip] :
	     info.realtime_duration_per_execution_instance) {
		for (auto const& [chip_on_connection, duration] : duration_per_chip) {
			os << "\t\t" << instance << ", " << chip_on_connection << ": "
			   << hate::to_string(duration) << "\n";
		}
	}
	os << ")";
	return os;
}

} // namespace grenade::vx::signal_flow
