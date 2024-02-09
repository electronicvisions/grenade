#include "grenade/vx/execution/detail/execution_instance_data.h"

#include <stdexcept>

namespace grenade::vx::execution::detail {

ExecutionInstanceData::ExecutionInstanceData(
    signal_flow::InputData const& input_data, signal_flow::Data const& global_output_data) :
    m_input_data(input_data), m_global_output_data(global_output_data), m_local_data()
{}

void ExecutionInstanceData::insert(
    signal_flow::Graph::vertex_descriptor descriptor,
    signal_flow::Graph::vertex_descriptor reference_descriptor,
    bool is_output)
{
	m_reference.insert({descriptor, reference_descriptor});
	if (is_output) {
		m_is_output.insert(descriptor);
	}
}

signal_flow::Data::Entry ExecutionInstanceData::extract_at(
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
	    "ExecutionInstanceData doesn't contain entry for descriptor " + std::to_string(descriptor) +
	    ".");
}

signal_flow::Data::Entry const& ExecutionInstanceData::at(
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
	    "ExecutionInstanceData doesn't contain entry for descriptor " + std::to_string(descriptor) +
	    ".");
}

std::vector<std::map<common::ExecutionInstanceID, common::Time>> const&
ExecutionInstanceData::get_runtime() const
{
	return m_input_data.runtime;
}

size_t ExecutionInstanceData::batch_size() const
{
	return m_input_data.batch_size();
}

signal_flow::Data ExecutionInstanceData::done()
{
	signal_flow::Data ret;
	for (auto const& descriptor : m_is_output) {
		ret.data.emplace(descriptor, extract_at(descriptor));
	}
	m_local_data.clear();
	m_is_output.clear();
	return ret;
}

} // namespace grenade::vx::execution::detail
