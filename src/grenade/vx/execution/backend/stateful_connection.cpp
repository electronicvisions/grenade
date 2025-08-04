#include "grenade/vx/execution/backend/stateful_connection.h"

namespace grenade::vx::execution::backend {

StatefulConnection::StatefulConnection(
    InitializedConnection&& connection, bool const enable_differential_config) :
    m_initialized_connection(std::move(connection)),
    m_state_storage(enable_differential_config, m_initialized_connection)
{
}

StatefulConnection::StatefulConnection(bool const enable_differential_config) :
    StatefulConnection(InitializedConnection(), enable_differential_config)
{
}

hxcomm::ConnectionTimeInfo StatefulConnection::get_time_info() const
{
	return m_initialized_connection.get_time_info();
}

std::string StatefulConnection::get_unique_identifier(
    std::optional<std::string> const& hwdb_path) const
{
	return m_initialized_connection.get_unique_identifier(hwdb_path);
}

std::string StatefulConnection::get_bitfile_info() const
{
	return m_initialized_connection.get_bitfile_info();
}

std::string StatefulConnection::get_remote_repo_state() const
{
	return m_initialized_connection.get_remote_repo_state();
}

hxcomm::HwdbEntry StatefulConnection::get_hwdb_entry() const
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
	return m_state_storage.config.get_enable_differential_config();
}

void StatefulConnection::set_enable_differential_config(bool const value)
{
	m_state_storage.config.set_enable_differential_config(value);
}

} // namespace grenade::vx::execution::backend
