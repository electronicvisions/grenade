#include "grenade/vx/execution/detail/execution_instance_snippet_data.h"

#include <stdexcept>

namespace grenade::vx::execution::detail {

ExecutionInstanceSnippetData::ExecutionInstanceSnippetData(
    grenade::common::OutputData const& global_results) :
    m_global_results(global_results), m_local_results()
{
}

void ExecutionInstanceSnippetData::insert(
    grenade::common::PortOnTopology descriptor,
    grenade::common::PortOnTopology reference_descriptor,
    bool is_output)
{
	m_reference.insert({descriptor, reference_descriptor});
	if (is_output) {
		m_is_output.insert(descriptor);
	}
}

grenade::common::PortData const& ExecutionInstanceSnippetData::at(
    grenade::common::PortOnTopology descriptor) const
{
	if (m_local_results.ports.contains(descriptor)) {
		return dynamic_cast<grenade::common::PortData const&>(
		    m_local_results.ports.get(descriptor));
	} else if (m_global_results.ports.contains(descriptor)) {
		return dynamic_cast<grenade::common::PortData const&>(
		    m_global_results.ports.get(descriptor));
	} else if (m_reference.contains(descriptor)) {
		return at(m_reference.at(descriptor));
	}
	std::stringstream ss;
	ss << "ExecutionInstanceSnippetData doesn't contain entry for descriptor " << descriptor << ".";
	throw std::out_of_range(ss.str());
}

size_t ExecutionInstanceSnippetData::batch_size() const
{
	return m_global_results.batch_size();
}

grenade::common::OutputData ExecutionInstanceSnippetData::done()
{
	grenade::common::OutputData ret;
	for (auto const& descriptor : m_is_output) {
		ret.ports.set(descriptor, at(descriptor));
	}
	m_local_results = grenade::common::OutputData();
	m_is_output.clear();
	return ret;
}

} // namespace grenade::vx::execution::detail
