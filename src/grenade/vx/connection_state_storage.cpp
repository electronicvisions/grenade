#include "grenade/vx/connection_state_storage.h"

#include "grenade/vx/backend/connection.h"

namespace grenade::vx {

ConnectionStateStorage::ConnectionStateStorage(
    bool const enable_differential_config, backend::Connection& connection) :
    enable_differential_config(enable_differential_config),
    current_config(),
    current_config_words(),
    reinit_base(connection.create_reinit_stack_entry()),
    reinit_differential(connection.create_reinit_stack_entry()),
    reinit_schedule_out_replacement(connection.create_reinit_stack_entry()),
    reinit_capmem_settling_wait(connection.create_reinit_stack_entry()),
    reinit_trigger(connection.create_reinit_stack_entry()),
    mutex()
{}

} // namespace grenade::vx
