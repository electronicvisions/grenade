#pragma once
#include "fisch/vx/word_access/type/omnibus.h"
#include "grenade/vx/execution/detail/connection_config.h"
#include "lola/vx/v3/chip.h"
#include "stadls/vx/v3/reinit_stack_entry.h"
#include <mutex>
#include <vector>

namespace grenade::vx::execution::backend {
struct Connection;
} // namespace grenade::vx::execution::backend

namespace grenade::vx::execution::detail {

struct ConnectionStateStorage
{
	bool enable_differential_config;
	lola::vx::v3::Chip current_config;
	std::vector<fisch::vx::word_access_type::Omnibus> current_config_words;

	ConnectionConfig config;

	stadls::vx::v3::ReinitStackEntry reinit_base;
	stadls::vx::v3::ReinitStackEntry reinit_differential;
	stadls::vx::v3::ReinitStackEntry reinit_schedule_out_replacement;
	stadls::vx::v3::ReinitStackEntry reinit_capmem_settling_wait;
	stadls::vx::v3::ReinitStackEntry reinit_trigger;
	std::mutex mutex;

	/**
	 * Construct connection state storage with differential mode flag and connection.
	 * The connection is used to construct the reinit stack entries.
	 * @param enable_differential_config Whether to enable differential config mode for this
	 * connection
	 * @param connection Connection to construct reinit stack entries from
	 */
	ConnectionStateStorage(bool enable_differential_config, backend::Connection& connection)
	    SYMBOL_VISIBLE;
};

} // namespace grenade::vx::execution::detail
