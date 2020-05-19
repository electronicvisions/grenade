#include "grenade/vx/execution_instance.h"

namespace grenade::vx::coordinate {

ExecutionInstance::ExecutionInstance(
    ExecutionIndex const execution_index, halco::hicann_dls::vx::DLSGlobal const dls) :
    m_execution_index(execution_index), m_dls_global(dls)
{}

halco::hicann_dls::vx::DLSGlobal ExecutionInstance::toDLSGlobal() const
{
	return m_dls_global;
}

ExecutionIndex ExecutionInstance::toExecutionIndex() const
{
	return m_execution_index;
}

bool ExecutionInstance::operator==(ExecutionInstance const& other) const
{
	return std::tie(m_execution_index, m_dls_global) ==
	       std::tie(other.m_execution_index, m_dls_global);
}

bool ExecutionInstance::operator!=(ExecutionInstance const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, ExecutionInstance const& instance)
{
	return (
	    os << "ExecutionInstance(" << instance.m_execution_index << ", " << instance.m_dls_global
	       << ")");
}

size_t hash_value(ExecutionInstance const& e)
{
	size_t hash = 0;
	boost::hash_combine(hash, boost::hash<ExecutionIndex>()(e.m_execution_index));
	boost::hash_combine(hash, boost::hash<halco::hicann_dls::vx::DLSGlobal>()(e.m_dls_global));
	return hash;
}

} // namespace grenade::vx::coordinate
