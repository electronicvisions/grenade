#pragma once
#include "hate/visibility.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/vx/connection_variant.h"
#include "stadls/vx/run_time_info.h"
#include "stadls/vx/v3/init_generator.h"
#include "stadls/vx/v3/playback_program.h"
#include "stadls/vx/v3/reinit_stack_entry.h"
#include <optional>
#include <string>
#include <variant>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include "pyhxcomm/common/managed_connection.h"
#endif

namespace grenade::vx::backend {

struct Connection;
stadls::vx::RunTimeInfo run(Connection&, stadls::vx::v3::PlaybackProgram&);
stadls::vx::RunTimeInfo run(Connection&, stadls::vx::v3::PlaybackProgram&&);


/**
 * Connection to hardware/simulation ensuring proper initialisation.
 */
struct Connection
{
	static constexpr char name[] = "Connection";

	/** Accepted initialization generators. */
	typedef std::variant<stadls::vx::v3::ExperimentInit, stadls::vx::v3::DigitalInit> Init;

	Connection() SYMBOL_VISIBLE;
	Connection(hxcomm::vx::ConnectionVariant&& connection) SYMBOL_VISIBLE;
	Connection(hxcomm::vx::ConnectionVariant&& connection, Init const& init) SYMBOL_VISIBLE;

	hxcomm::ConnectionTimeInfo get_time_info() const SYMBOL_VISIBLE;

	std::string get_unique_identifier(std::optional<std::string> const& hwdb_path) const
	    SYMBOL_VISIBLE;

	hxcomm::vx::ConnectionVariant&& release() SYMBOL_VISIBLE;

	stadls::vx::v3::ReinitStackEntry create_reinit_stack_entry() SYMBOL_VISIBLE;

	bool is_quiggeldy() const SYMBOL_VISIBLE;

private:
	hxcomm::vx::ConnectionVariant m_connection;
	size_t m_expected_link_notification_count;
	stadls::vx::v3::ReinitStackEntry m_init;

	friend stadls::vx::RunTimeInfo run(Connection&, stadls::vx::v3::PlaybackProgram&);
	friend stadls::vx::RunTimeInfo run(Connection&, stadls::vx::v3::PlaybackProgram&&);
};

} // namespace grenade::vx::backend

GENPYBIND_MANUAL({
	pyhxcomm::ManagedPyBind11Helper<grenade::vx::backend::Connection> helper(
	    parent, BOOST_HANA_STRING("Connection"));
})
