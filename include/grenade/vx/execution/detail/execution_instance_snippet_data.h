#pragma once
#include "grenade/vx/signal_flow/data_snippet.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input_data_snippet.h"
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
	ExecutionInstanceSnippetData(
	    signal_flow::InputDataSnippet const& input_data,
	    signal_flow::DataSnippet const& global_output_data) SYMBOL_VISIBLE;

	/**
	 * Insert entry by reference without data copy.
	 * @param descriptor Descriptor to insert for
	 * @param reference_descriptor Descriptor to use as data reference
	 * @param is_output Whether data shall be present in output data structure
	 */
	void insert(
	    signal_flow::Graph::vertex_descriptor descriptor,
	    signal_flow::Graph::vertex_descriptor reference_descriptor,
	    bool is_output = false) SYMBOL_VISIBLE;

	/**
	 * Insert entry with data copy or move.
	 * @param descriptor Descriptor to insert for
	 * @param entry Data entry to add
	 * @param is_output Whether data shall be present in output data structure
	 */
	template <typename EntryT>
	void insert(
	    signal_flow::Graph::vertex_descriptor descriptor, EntryT&& entry, bool is_output = false);

	signal_flow::DataSnippet::Entry const& at(
	    signal_flow::Graph::vertex_descriptor descriptor) const SYMBOL_VISIBLE;

	/**
	 * Get runtime from input data with batch entry as outer dimension.
	 */
	std::vector<std::map<grenade::common::ExecutionInstanceID, common::Time>> const& get_runtime()
	    const SYMBOL_VISIBLE;

	size_t batch_size() const SYMBOL_VISIBLE;

	/**
	 * Extract output data.
	 * Empties local data storage.
	 */
	signal_flow::DataSnippet done() SYMBOL_VISIBLE;

	signal_flow::InputDataSnippet const& get_input_data() const SYMBOL_VISIBLE;

private:
	signal_flow::DataSnippet::Entry extract_at(signal_flow::Graph::vertex_descriptor descriptor);

	signal_flow::InputDataSnippet const& m_input_data;
	signal_flow::DataSnippet const& m_global_output_data;
	signal_flow::DataSnippet m_local_data;
	std::map<signal_flow::Graph::vertex_descriptor, signal_flow::Graph::vertex_descriptor>
	    m_reference;
	std::set<signal_flow::Graph::vertex_descriptor> m_is_output;
};

} // namespace grenade::vx::execution::detail

#include "grenade/vx/execution/detail/execution_instance_snippet_data.tcc"
