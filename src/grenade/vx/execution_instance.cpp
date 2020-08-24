#include "grenade/vx/execution_instance.h"

#include "grenade/cerealization.h"
#include "halco/common/cerealization_geometry.h"

namespace grenade::vx::coordinate {

ExecutionInstance::ExecutionInstance(
    ExecutionIndex const execution_index, halco::hicann_dls::vx::v2::DLSGlobal const dls) :
    m_execution_index(execution_index), m_dls_global(dls)
{}

halco::hicann_dls::vx::v2::DLSGlobal ExecutionInstance::toDLSGlobal() const
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
	boost::hash_combine(hash, boost::hash<halco::hicann_dls::vx::v2::DLSGlobal>()(e.m_dls_global));
	return hash;
}

template <typename Archive>
void ExecutionInstance::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_execution_index);
	ar(m_dls_global);
}

} // namespace grenade::vx::coordinate

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::coordinate::ExecutionInstance)
CEREAL_CLASS_VERSION(grenade::vx::coordinate::ExecutionInstance, 0)
