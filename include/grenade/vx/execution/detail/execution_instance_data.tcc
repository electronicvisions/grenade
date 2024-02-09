#pragma once
#include "grenade/vx/execution/detail/execution_instance_data.h"

namespace grenade::vx::execution::detail {

template <typename EntryT>
void ExecutionInstanceData::insert(
    signal_flow::Graph::vertex_descriptor descriptor, EntryT&& entry, bool is_output)
{
	m_local_data.data.emplace(descriptor, std::forward<EntryT>(entry));
	if (is_output) {
		m_is_output.insert(descriptor);
	}
}

} // namespace grenade::vx::execution::detail
