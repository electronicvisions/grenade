#include "grenade/vx/common/execution_instance_id.h"

#include <ostream>

namespace grenade::vx::common {

ExecutionInstanceID::ExecutionInstanceID(
    ExecutionIndex const execution_index, halco::hicann_dls::vx::v3::DLSGlobal const dls) :
    m_execution_index(execution_index), m_dls_global(dls)
{}

halco::hicann_dls::vx::v3::DLSGlobal ExecutionInstanceID::toDLSGlobal() const
{
	return m_dls_global;
}

ExecutionIndex ExecutionInstanceID::toExecutionIndex() const
{
	return m_execution_index;
}

bool ExecutionInstanceID::operator==(ExecutionInstanceID const& other) const
{
	return std::tie(m_execution_index, m_dls_global) ==
	       std::tie(other.m_execution_index, other.m_dls_global);
}

bool ExecutionInstanceID::operator!=(ExecutionInstanceID const& other) const
{
	return !(*this == other);
}

size_t ExecutionInstanceID::hash() const
{
	// We include the type name in the hash to reduce the number of hash collisions in
	// python code, where __hash__ is used in heterogeneous containers.
	static const size_t seed = boost::hash_value(typeid(ExecutionInstanceID).name());
	size_t hash = seed;
	boost::hash_combine(hash, hash_value(*this));
	return hash;
}

std::ostream& operator<<(std::ostream& os, ExecutionInstanceID const& instance)
{
	return (
	    os << "ExecutionInstanceID(" << instance.m_execution_index << ", " << instance.m_dls_global
	       << ")");
}

size_t hash_value(ExecutionInstanceID const& e)
{
	size_t hash = 0;
	boost::hash_combine(hash, boost::hash<ExecutionIndex>()(e.m_execution_index));
	boost::hash_combine(hash, boost::hash<halco::hicann_dls::vx::v3::DLSGlobal>()(e.m_dls_global));
	return hash;
}

} // namespace grenade::vx::common

namespace std {

size_t hash<grenade::vx::common::ExecutionInstanceID>::operator()(
    grenade::vx::common::ExecutionInstanceID const& t) const
{
	return grenade::vx::common::hash_value(t);
}

} // namespace std
