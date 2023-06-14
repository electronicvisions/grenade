#include "grenade/vx/signal_flow/execution_instance.h"

#include <ostream>

namespace grenade::vx::signal_flow {

ExecutionInstance::ExecutionInstance(
    ExecutionIndex const execution_index, halco::hicann_dls::vx::v3::DLSGlobal const dls) :
    m_execution_index(execution_index), m_dls_global(dls)
{}

halco::hicann_dls::vx::v3::DLSGlobal ExecutionInstance::toDLSGlobal() const
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
	       std::tie(other.m_execution_index, other.m_dls_global);
}

bool ExecutionInstance::operator!=(ExecutionInstance const& other) const
{
	return !(*this == other);
}

size_t ExecutionInstance::hash() const
{
	// We include the type name in the hash to reduce the number of hash collisions in
	// python code, where __hash__ is used in heterogeneous containers.
	static const size_t seed = boost::hash_value(typeid(ExecutionInstance).name());
	size_t hash = seed;
	boost::hash_combine(hash, hash_value(*this));
	return hash;
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
	boost::hash_combine(hash, boost::hash<halco::hicann_dls::vx::v3::DLSGlobal>()(e.m_dls_global));
	return hash;
}

} // namespace grenade::vx::signal_flow

namespace std {

size_t hash<grenade::vx::signal_flow::ExecutionInstance>::operator()(
    grenade::vx::signal_flow::ExecutionInstance const& t) const
{
	return grenade::vx::signal_flow::hash_value(t);
}

} // namespace std
