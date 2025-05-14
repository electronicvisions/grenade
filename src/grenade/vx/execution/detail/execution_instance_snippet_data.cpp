#include "grenade/vx/execution/detail/execution_instance_snippet_data.h"

#include <stdexcept>

namespace grenade::vx::execution::detail {

ExecutionInstanceSnippetData::ExecutionInstanceSnippetData(
    signal_flow::InputDataSnippet const& input_data,
    signal_flow::DataSnippet const& global_output_data) :
    m_input_data(input_data), m_global_output_data(global_output_data), m_local_data()
{
}

void ExecutionInstanceSnippetData::insert(
    signal_flow::Graph::vertex_descriptor descriptor,
    signal_flow::Graph::vertex_descriptor reference_descriptor,
    bool is_output)
{
	m_reference.insert({descriptor, reference_descriptor});
	if (is_output) {
		m_is_output.insert(descriptor);
	}
}

signal_flow::DataSnippet::Entry ExecutionInstanceSnippetData::extract_at(
    signal_flow::Graph::vertex_descriptor descriptor)
{
	if (m_local_data.data.contains(descriptor)) {
		return std::move(m_local_data.data.at(descriptor));
	} else if (m_input_data.data.contains(descriptor)) {
		return m_input_data.data.at(descriptor);
	} else if (m_global_output_data.data.contains(descriptor)) {
		return m_global_output_data.data.at(descriptor);
	} else if (m_reference.contains(descriptor)) {
		return extract_at(m_reference.at(descriptor));
	}
	throw std::out_of_range(
	    "ExecutionInstanceSnippetData doesn't contain entry for descriptor " +
	    std::to_string(descriptor) + ".");
}

signal_flow::DataSnippet::Entry const& ExecutionInstanceSnippetData::at(
    signal_flow::Graph::vertex_descriptor descriptor) const
{
	if (m_local_data.data.contains(descriptor)) {
		return m_local_data.data.at(descriptor);
	} else if (m_input_data.data.contains(descriptor)) {
		return m_input_data.data.at(descriptor);
	} else if (m_global_output_data.data.contains(descriptor)) {
		return m_global_output_data.data.at(descriptor);
	} else if (m_reference.contains(descriptor)) {
		return at(m_reference.at(descriptor));
	}
	throw std::out_of_range(
	    "ExecutionInstanceSnippetData doesn't contain entry for descriptor " +
	    std::to_string(descriptor) + ".");
}

std::vector<std::map<grenade::common::ExecutionInstanceID, common::Time>> const&
ExecutionInstanceSnippetData::get_runtime() const
{
	return m_input_data.runtime;
}

size_t ExecutionInstanceSnippetData::batch_size() const
{
	return m_input_data.batch_size();
}

signal_flow::DataSnippet ExecutionInstanceSnippetData::done()
{
	signal_flow::DataSnippet ret;
	for (auto const& descriptor : m_is_output) {
		ret.data.emplace(descriptor, extract_at(descriptor));
	}
	m_local_data.clear();
	m_is_output.clear();
	return ret;
}

} // namespace grenade::vx::execution::detail
