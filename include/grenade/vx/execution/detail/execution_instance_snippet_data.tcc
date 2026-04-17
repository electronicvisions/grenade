#pragma once
#include "grenade/vx/execution/detail/execution_instance_snippet_data.h"

namespace grenade::vx::execution::detail {

template <typename EntryT>
void ExecutionInstanceSnippetData::insert(
    grenade::common::PortOnTopology descriptor, EntryT&& entry, bool is_output)
{
	m_local_results.ports.set(descriptor, std::forward<EntryT>(entry));
	if (is_output) {
		m_is_output.insert(descriptor);
	}
}

} // namespace grenade::vx::execution::detail
