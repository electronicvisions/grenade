#pragma once
#include "grenade/vx/execution/backend/detail/stateful_connection_config.h"
#include "grenade/vx/execution/backend/initialized_connection.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include "hxcomm/common/connection_time_info.h"
#include "stadls/vx/v3/reinit_stack_entry.h"
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace grenade::vx::execution::backend {

struct PlaybackProgram;
struct StatefulConnection;
struct RunTimeInfo;
RunTimeInfo run(StatefulConnection&, PlaybackProgram&);
RunTimeInfo run(StatefulConnection&, PlaybackProgram&&);


/**
 * Connection to hardware/simulation ensuring proper initialization and tracking of state during and
 * across possibly non-contiguous execution of playback programs.
 */
struct StatefulConnection
{
	/**
	 * Construct connection with default-initialized InitializedConnection.
	 * @param enable_differential_config Whether to enable tracking hardware configurations and only
	 * applying differential changes.
	 */
	StatefulConnection(bool enable_differential_config = true) SYMBOL_VISIBLE;

	/**
	 * Construct connection from initialized connection. Ownership of the initialized connection is
	 * transferred to this object.
	 * @param connection InitializedConnection to use
	 * @param enable_differential_config Whether to enable tracking hardware configurations and only
	 * applying differential changes.
	 */
	StatefulConnection(InitializedConnection&& connection, bool enable_differential_config = true)
	    SYMBOL_VISIBLE;

	/**
	 * Get time information of execution(s).
	 * @return Time information
	 */
	hxcomm::ConnectionTimeInfo get_time_info() const SYMBOL_VISIBLE;

	/**
	 * Get unique identifier from hwdb.
	 * @param hwdb_path Optional path to hwdb
	 * @return Unique identifier
	 */
	std::string get_unique_identifier(std::optional<std::string> const& hwdb_path) const
	    SYMBOL_VISIBLE;

	/**
	 * Get bitfile information.
	 * @return Bitfile info
	 */
	std::string get_bitfile_info() const SYMBOL_VISIBLE;

	/**
	 * Get server-side remote repository state information.
	 * Only non-empty for connection being QuiggeldyConnection.
	 * @return Repository state
	 */
	std::string get_remote_repo_state() const SYMBOL_VISIBLE;

	/**
	 * Get hwdb entry information.
	 * @return Hwdb entry
	 */
	hxcomm::HwdbEntry get_hwdb_entry() const SYMBOL_VISIBLE;

	/**
	 * Release ownership of initialized connection.
	 * @return Previously owned initialized connection
	 */
	InitializedConnection&& release() SYMBOL_VISIBLE;

	/**
	 * Get whether owned hxcomm connection is QuiggeldyConnection.
	 * @return Boolean value
	 */
	bool is_quiggeldy() const SYMBOL_VISIBLE;

	/**
	 * Get whether tracking hardware configurations and only applying differential changes is
	 * enabled.
	 */
	bool get_enable_differential_config() const SYMBOL_VISIBLE;

	/**
	 * Set whether to enable tracking hardware configurations and only applying differential
	 * changes.
	 * @param value Value to set
	 */
	void set_enable_differential_config(bool value) SYMBOL_VISIBLE;

private:
	InitializedConnection m_initialized_connection;

	/**
	 * Tracked hardware configuration.
	 */
	detail::StatefulConnectionConfig m_config;

	/**
	 * Reinit applying the base configuration.
	 */
	stadls::vx::v3::ReinitStackEntry m_reinit_base;

	/**
	 * Reinit applying the differential configuration.
	 */
	stadls::vx::v3::ReinitStackEntry m_reinit_differential;

	/**
	 * Reinit applying reading the hardware state when the user gets scheduled out by Quiggeldy and
	 * afterwards applying this state upon the next execution.
	 */
	stadls::vx::v3::ReinitStackEntry m_reinit_schedule_out_replacement;

	/**
	 * Reinit waiting for the CapMem to settle.
	 */
	stadls::vx::v3::ReinitStackEntry m_reinit_capmem_settling_wait;

	/**
	 * Reinit starting the PPUs.
	 */
	stadls::vx::v3::ReinitStackEntry m_reinit_start_ppus;

	/**
	 * Mutex used for exclusive usage of the connection in run().
	 */
	std::unique_ptr<std::mutex> m_mutex;


	friend RunTimeInfo run(StatefulConnection&, PlaybackProgram&);
	friend RunTimeInfo run(StatefulConnection&, PlaybackProgram&&);
};

} // namespace grenade::vx::execution::backend
