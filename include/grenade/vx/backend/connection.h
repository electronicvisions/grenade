#pragma once
#include "hate/visibility.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/vx/connection_variant.h"
#include "stadls/vx/run_time_info.h"
#include "stadls/vx/v2/init_generator.h"
#include "stadls/vx/v2/playback_program.h"
#include "stadls/vx/v2/reinit_stack_entry.h"
#include <optional>
#include <string>
#include <variant>

namespace grenade::vx::backend {

struct Connection;
stadls::vx::RunTimeInfo run(Connection&, stadls::vx::v2::PlaybackProgram&);
stadls::vx::RunTimeInfo run(Connection&, stadls::vx::v2::PlaybackProgram&&);


/**
 * Connection to hardware/simulation ensuring proper initialisation.
 */
struct Connection
{
	/** Accepted initialization generators. */
	typedef std::variant<stadls::vx::v2::ExperimentInit, stadls::vx::v2::DigitalInit> Init;

	Connection() SYMBOL_VISIBLE;
	Connection(hxcomm::vx::ConnectionVariant&& connection) SYMBOL_VISIBLE;
	Connection(hxcomm::vx::ConnectionVariant&& connection, Init const& init) SYMBOL_VISIBLE;

	hxcomm::ConnectionTimeInfo get_time_info() const SYMBOL_VISIBLE;

	std::string get_unique_identifier(std::optional<std::string> const& hwdb_path) const
	    SYMBOL_VISIBLE;

	hxcomm::vx::ConnectionVariant&& release() SYMBOL_VISIBLE;

	stadls::vx::v2::ReinitStackEntry create_reinit_stack_entry() SYMBOL_VISIBLE;

private:
	hxcomm::vx::ConnectionVariant m_connection;
	size_t m_expected_link_notification_count;

	friend stadls::vx::RunTimeInfo run(Connection&, stadls::vx::v2::PlaybackProgram&);
	friend stadls::vx::RunTimeInfo run(Connection&, stadls::vx::v2::PlaybackProgram&&);
};

} // namespace grenade::vx::backend
