#pragma once
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/execution/backend/stateful_connection.h"
#include "grenade/vx/execution/execution_instance_hooks.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"
#include <memory>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include "pyhxcomm/common/managed_connection.h"
#endif

namespace grenade::vx::signal_flow {
struct InputData;
struct OutputData;
struct ExecutionInstanceHooks;
class Graph;
} // namespace grenade::v::signal_flowx

namespace grenade::vx {
namespace execution GENPYBIND_TAG_GRENADE_VX_EXECUTION {

class JITGraphExecutor;

/**
 * Just-in-time graph executor.
 * No partial playback stream is pre-generated and nothing is reused for another run with e.g.
 * different input. This especially allows implementation without promise support, since before
 * processing each new chip instance, all necessary results are already available by value.
 */
class JITGraphExecutor
{
public:
	typedef std::tuple<bool> init_parameters_type;

	static constexpr char name[] = "JITGraphExecutor";

	typedef std::map<
	    grenade::common::ExecutionInstanceID,
	    std::map<common::ChipOnConnection, lola::vx::v3::Chip>>
	    ChipConfigs;

	typedef std::map<grenade::common::ExecutionInstanceID, std::shared_ptr<ExecutionInstanceHooks>>
	    Hooks;

	/**
	 * Construct executor with active connections from environment.
	 * @param enable_differential_config Whether to enable differential configuration writes instead
	 * of full ones
	 */
	JITGraphExecutor(bool enable_differential_config = true) SYMBOL_VISIBLE;

	/**
	 * Construct executor with given active connections.
	 * @param connections InitializedConnections to acquire and provide
	 */
	JITGraphExecutor(std::map<grenade::common::ConnectionOnExecutor, backend::StatefulConnection>&&
	                     connections) SYMBOL_VISIBLE;

	/**
	 * Get identifiers of connections contained in executor.
	 * @return Set of connection identifiers
	 */
	std::set<grenade::common::ConnectionOnExecutor> contained_connections() const SYMBOL_VISIBLE;

	/**
	 * Release contained connections.
	 * @return InitializedConnections to the associated hardware
	 */
	std::map<grenade::common::ConnectionOnExecutor, backend::StatefulConnection>&&
	release_connections() SYMBOL_VISIBLE;

	std::map<grenade::common::ConnectionOnExecutor, std::vector<hxcomm::ConnectionTimeInfo>>
	get_time_info() const SYMBOL_VISIBLE;

	std::map<grenade::common::ConnectionOnExecutor, std::vector<std::string>> get_unique_identifier(
	    std::optional<std::string> const& hwdb_path) const SYMBOL_VISIBLE;

	std::map<grenade::common::ConnectionOnExecutor, std::vector<std::string>> get_bitfile_info()
	    const SYMBOL_VISIBLE;

	std::map<grenade::common::ConnectionOnExecutor, std::vector<std::string>>
	get_remote_repo_state() const SYMBOL_VISIBLE;

	std::map<grenade::common::ConnectionOnExecutor, std::vector<hxcomm::HwdbEntry>> get_hwdb_entry()
	    const SYMBOL_VISIBLE;

private:
	std::map<grenade::common::ConnectionOnExecutor, backend::StatefulConnection> m_connections;

	/**
	 * Check whether the given graph can be executed.
	 * @param graph Graph instance
	 */
	bool is_executable_on(signal_flow::Graph const& graph);

	/**
	 * Check that graph can be executed.
	 * This function combines `is_executable_on` and
	 * `has_dangling_inputs`.
	 * @param graph Graph to check
	 */
	void check(signal_flow::Graph const& graph);

	friend signal_flow::OutputData run(
	    JITGraphExecutor& executor,
	    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
	    std::vector<std::reference_wrapper<ChipConfigs const>> const& configs,
	    signal_flow::InputData const& inputs,
	    Hooks&& hooks);
};

GENPYBIND_MANUAL({
	pyhxcomm::ManagedPyBind11Helper<grenade::vx::execution::JITGraphExecutor> helper(
	    parent, BOOST_HANA_STRING("JITGraphExecutor"));
})

} // namespace execution
} // namespace grenade::vx
