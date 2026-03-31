#include "grenade/vx/execution/backend/stateful_connection.h"
#include "grenade/vx/execution/detail/system.h"

namespace grenade::vx::execution::backend {

StatefulConnection::StatefulConnection(
    InitializedConnection&& connection,
    std::optional<std::vector<bool>> const& enable_differential_config) :
    m_initialized_connection(std::move(connection)),
    m_configs([this, enable_differential_config]() {
	    std::map<common::ChipOnConnection, detail::StatefulChipConfig> ret;
	    auto const hwdb_entries = get_hwdb_entry();
	    auto const chips_on_connection = get_chips_on_connection();
	    assert(hwdb_entries.size() == chips_on_connection.size());
	    for (size_t i = 0; i < chips_on_connection.size(); i++) {
		    if (std::holds_alternative<hwdb4cpp::HXCubeSetupEntry>(hwdb_entries.at(i))) {
			    if (!std::holds_alternative<hwdb4cpp::HXCubeSetupEntry>(hwdb_entries.at(0))) {
				    throw std::logic_error("Heterogeneous hardware types.");
			    }
			    ret.emplace(chips_on_connection.at(i), lola::vx::v3::ChipAndSinglechipFPGA());
		    } else if (std::holds_alternative<hwdb4cpp::JboaSetupEntry>(hwdb_entries.at(i))) {
			    if (!std::holds_alternative<hwdb4cpp::JboaSetupEntry>(hwdb_entries.at(0))) {
				    throw std::logic_error("Heterogeneous hardware types.");
			    }
			    ret.emplace(
			        chips_on_connection.at(i), lola::vx::v3::ChipAndMultichipJboaLeafFPGA());
		    } else {
			    throw std::logic_error("Invalid hwdb entry.");
		    }
		    ret.at(chips_on_connection.at(i))
		        .set_enable_differential_config(
		            enable_differential_config ? enable_differential_config->at(i) : true);
	    }

	    return ret;
    }()),
    m_reinit_base(m_initialized_connection.create_reinit_stack_entry()),
    m_reinit_differential(m_initialized_connection.create_reinit_stack_entry()),
    m_reinit_schedule_out_replacement(m_initialized_connection.create_reinit_stack_entry()),
    m_reinit_capmem_settling_wait(m_initialized_connection.create_reinit_stack_entry()),
    m_reinit_start_ppus(m_initialized_connection.create_reinit_stack_entry()),
    m_mutex(std::make_unique<std::mutex>())
{
}

StatefulConnection::StatefulConnection(
    std::optional<std::vector<bool>> const& enable_differential_config) :
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

std::vector<bool> StatefulConnection::get_enable_differential_config() const
{
	std::vector<bool> ret;
	for (auto const& [_, config] : m_configs) {
		ret.push_back(config.get_enable_differential_config());
	}
	return ret;
}

void StatefulConnection::set_enable_differential_config(std::vector<bool> values)
{
	if (values.size() != m_configs.size()) {
		throw std::runtime_error("Wrong number of differential config flags.");
	}
	auto const chips_on_connection = get_chips_on_connection();
	for (size_t i = 0; i < values.size(); i++) {
		m_configs.at(chips_on_connection.at(i)).set_enable_differential_config(values.at(i));
	}
}

std::vector<common::ChipOnConnection> StatefulConnection::get_chips_on_connection() const
{
	return m_initialized_connection.get_chips_on_connection();
}

} // namespace grenade::vx::execution::backend
