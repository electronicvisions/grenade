#include "grenade/vx/execution/backend/stateful_connection.h"
#include "grenade/vx/execution/detail/system.h"

namespace grenade::vx::execution::backend {

StatefulConnection::StatefulConnection(
    InitializedConnection&& connection, bool const enable_differential_config) :
    m_initialized_connection(std::move(connection)),
    m_config([this]() -> grenade::vx::execution::detail::System {
	    auto hwdb_entries = get_hwdb_entry();

	    if (std::holds_alternative<hwdb4cpp::HXCubeSetupEntry>(hwdb_entries.at(0))) {
		    return lola::vx::v3::ChipAndSinglechipFPGA();
	    } else if (std::holds_alternative<hwdb4cpp::JboaSetupEntry>(hwdb_entries.at(0))) {
		    return lola::vx::v3::ChipAndMultichipJboaLeafFPGA();
	    } else {
		    throw std::logic_error("Invalid hwdb entry.");
	    }
    }()),
    m_reinit_base(m_initialized_connection.create_reinit_stack_entry()),
    m_reinit_differential(m_initialized_connection.create_reinit_stack_entry()),
    m_reinit_schedule_out_replacement(m_initialized_connection.create_reinit_stack_entry()),
    m_reinit_capmem_settling_wait(m_initialized_connection.create_reinit_stack_entry()),
    m_reinit_start_ppus(m_initialized_connection.create_reinit_stack_entry()),
    m_mutex(std::make_unique<std::mutex>())
{
	m_config.set_enable_differential_config(enable_differential_config);
}

StatefulConnection::StatefulConnection(bool const enable_differential_config) :
    StatefulConnection(InitializedConnection(), enable_differential_config)
{
}

std::vector<hxcomm::ConnectionTimeInfo> StatefulConnection::get_time_info() const
{
	return m_initialized_connection.get_time_info();
}

std::vector<std::string> StatefulConnection::get_unique_identifier(
    std::optional<std::string> const& hwdb_path) const
{
	return m_initialized_connection.get_unique_identifier(hwdb_path);
}

std::vector<std::string> StatefulConnection::get_bitfile_info() const
{
	return m_initialized_connection.get_bitfile_info();
}

std::vector<std::string> StatefulConnection::get_remote_repo_state() const
{
	return m_initialized_connection.get_remote_repo_state();
}

std::vector<hxcomm::HwdbEntry> StatefulConnection::get_hwdb_entry() const
{
	return m_initialized_connection.get_hwdb_entry();
}

InitializedConnection&& StatefulConnection::release()
{
	return std::move(m_initialized_connection);
}

bool StatefulConnection::is_quiggeldy() const
{
	return m_initialized_connection.is_quiggeldy();
}

bool StatefulConnection::get_enable_differential_config() const
{
	return m_config.get_enable_differential_config();
}

void StatefulConnection::set_enable_differential_config(bool const value)
{
	m_config.set_enable_differential_config(value);
}

std::vector<common::ChipOnConnection> StatefulConnection::get_chips_on_connection() const
{
	return m_initialized_connection.get_chips_on_connection();
}

} // namespace grenade::vx::execution::backend
