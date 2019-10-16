#include "grenade/vx/execution_instance.h"

namespace grenade::vx::coordinate {

ExecutionInstance::ExecutionInstance(
    ExecutionIndex const temporal_index, halco::hicann_dls::vx::HemisphereGlobal const dls) :
    m_dls_global(dls), m_temporal_index(temporal_index)
{}

halco::hicann_dls::vx::HemisphereGlobal ExecutionInstance::toHemisphereGlobal() const
{
	return m_dls_global;
}

ExecutionIndex ExecutionInstance::toExecutionIndex() const
{
	return m_temporal_index;
}

bool ExecutionInstance::operator<(ExecutionInstance const& other) const
{
	return std::tie(m_temporal_index, m_dls_global) <
	       std::tie(other.m_temporal_index, m_dls_global);
}

bool ExecutionInstance::operator>(ExecutionInstance const& other) const
{
	return std::tie(m_temporal_index, m_dls_global) >
	       std::tie(other.m_temporal_index, m_dls_global);
}

bool ExecutionInstance::operator<=(ExecutionInstance const& other) const
{
	return !(*this > other);
}

bool ExecutionInstance::operator>=(ExecutionInstance const& other) const
{
	return !(*this < other);
}

bool ExecutionInstance::operator==(ExecutionInstance const& other) const
{
	return std::tie(m_temporal_index, m_dls_global) ==
	       std::tie(other.m_temporal_index, m_dls_global);
}

bool ExecutionInstance::operator!=(ExecutionInstance const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, ExecutionInstance const& instance)
{
	return (
	    os << "ExecutionInstance(" << instance.m_temporal_index << ", " << instance.m_dls_global
	       << ")");
}

} // namespace grenade::vx::coordinate
