#pragma once
#include "grenade/common/output_data.h"
#include "grenade/common/vertex.h"
#include "grenade/common/vertex_on_topology.h"
#include "hate/visibility.h"
#include <map>
#include <set>


namespace grenade::vx::execution::detail {

/**
 * Data storage for execution instance without unnecessary copies.
 *
 * It is used in the ExecutionInstanceSnippetRealtimeExecutor as temporary execution-instance-local
 * storage of data associated with vertices. During construction of the experiment and
 * post-processing of results only necessary copies are made. After successful post-processing, the
 * output data can be extracted to be placed into the global output data storage.
 */
struct ExecutionInstanceSnippetData
{
	ExecutionInstanceSnippetData(grenade::common::OutputData const& global_results) SYMBOL_VISIBLE;

	/**
	 * Insert entry by reference without data copy.
	 * @param descriptor Descriptor to insert for
	 * @param reference_descriptor Descriptor to use as data reference
	 * @param is_output Whether data shall be present in output data structure
	 */
	void insert(
	    grenade::common::PortOnTopology descriptor,
	    grenade::common::PortOnTopology reference_descriptor,
	    bool is_output = true) SYMBOL_VISIBLE;

	/**
	 * Insert entry with data copy or move.
	 * @param descriptor Descriptor to insert for
	 * @param entry Data entry to add
	 * @param is_output Whether data shall be present in output data structure
	 */
	template <typename EntryT>
	void insert(grenade::common::PortOnTopology descriptor, EntryT&& entry, bool is_output = true);

	grenade::common::PortData const& at(grenade::common::PortOnTopology descriptor) const
	    SYMBOL_VISIBLE;

	size_t batch_size() const SYMBOL_VISIBLE;

	/**
	 * Extract output data.
	 * Empties local data storage.
	 */
	grenade::common::OutputData done() SYMBOL_VISIBLE;

private:
	grenade::common::OutputData const& m_global_results;
	grenade::common::OutputData m_local_results;
	std::map<grenade::common::PortOnTopology, grenade::common::PortOnTopology> m_reference;
	std::set<grenade::common::PortOnTopology> m_is_output;
};

} // namespace grenade::vx::execution::detail

#include "grenade/vx/execution/detail/execution_instance_snippet_data.tcc"
