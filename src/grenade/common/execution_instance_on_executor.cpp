#include "grenade/common/execution_instance_on_executor.h"

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include <boost/functional/hash.hpp>

namespace grenade::common {

ExecutionInstanceOnExecutor::ExecutionInstanceOnExecutor(
    ExecutionInstanceID const& execution_instance_id,
    ConnectionOnExecutor const& connection_on_executor) :
    execution_instance_id(execution_instance_id), connection_on_executor(connection_on_executor)
{
}

bool ExecutionInstanceOnExecutor::operator==(ExecutionInstanceOnExecutor const& other) const
{
	return std::tie(execution_instance_id, connection_on_executor) ==
	       std::tie(other.execution_instance_id, other.connection_on_executor);
}

bool ExecutionInstanceOnExecutor::operator!=(ExecutionInstanceOnExecutor const& other) const
{
	return std::tie(execution_instance_id, connection_on_executor) !=
	       std::tie(other.execution_instance_id, other.connection_on_executor);
}

bool ExecutionInstanceOnExecutor::operator<=(ExecutionInstanceOnExecutor const& other) const
{
	return std::tie(execution_instance_id, connection_on_executor) <=
	       std::tie(other.execution_instance_id, other.connection_on_executor);
}

bool ExecutionInstanceOnExecutor::operator>=(ExecutionInstanceOnExecutor const& other) const
{
	return std::tie(execution_instance_id, connection_on_executor) >=
	       std::tie(other.execution_instance_id, other.connection_on_executor);
}

bool ExecutionInstanceOnExecutor::operator<(ExecutionInstanceOnExecutor const& other) const
{
	return std::tie(execution_instance_id, connection_on_executor) <
	       std::tie(other.execution_instance_id, other.connection_on_executor);
}

bool ExecutionInstanceOnExecutor::operator>(ExecutionInstanceOnExecutor const& other) const
{
	return std::tie(execution_instance_id, connection_on_executor) >
	       std::tie(other.execution_instance_id, other.connection_on_executor);
}

size_t ExecutionInstanceOnExecutor::hash() const
{
	size_t ret = std::hash<ExecutionInstanceID>{}(execution_instance_id);
	boost::hash_combine(ret, std::hash<ConnectionOnExecutor>{}(connection_on_executor));
	return ret;
}

std::ostream& operator<<(std::ostream& os, ExecutionInstanceOnExecutor const& value)
{
	return os << "ExecutionInstanceOnExecutor(" << value.execution_instance_id << ", "
	          << value.connection_on_executor << ")";
}

} // namespace grenade::common
