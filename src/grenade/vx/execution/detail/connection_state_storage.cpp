#include "grenade/vx/execution/detail/connection_state_storage.h"

#include "grenade/vx/execution/backend/initialized_connection.h"

namespace grenade::vx::execution::detail {

ConnectionStateStorage::ConnectionStateStorage(
    bool const enable_differential_config, backend::InitializedConnection& connection) :
    enable_differential_config(enable_differential_config),
    current_config(),
    current_config_words(),
    config(),
    reinit_base(connection.create_reinit_stack_entry()),
    reinit_differential(connection.create_reinit_stack_entry()),
    reinit_schedule_out_replacement(connection.create_reinit_stack_entry()),
    reinit_capmem_settling_wait(connection.create_reinit_stack_entry()),
    reinit_trigger(connection.create_reinit_stack_entry()),
    mutex()
{
	config.set_enable_differential_config(enable_differential_config);
}

} // namespace grenade::vx::execution::detail
