#include "grenade/vx/execution/backend/initialized_connection.h"

#include "grenade/vx/execution/backend/initialized_connection_run.h"
#include "halco/hicann-dls/vx/v3/barrier.h"
#include "halco/hicann-dls/vx/v3/highspeed_link.h"
#include "halco/hicann-dls/vx/v3/jtag.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/jtag.h"
#include "hate/timer.h"
#include "hxcomm/vx/connection_from_env.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/v3/run.h"
#include <log4cxx/logger.h>

namespace {

void perform_hardware_check(hxcomm::vx::ConnectionVariant& connection)
{
	if (std::holds_alternative<hxcomm::vx::MultiZeroMockConnection>(connection)) {
		return;
	}

	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	using namespace stadls::vx::v3;

	// perform hardware version check(s)
	PlaybackProgramBuilder builder;
	auto jtag_id_ticket = builder.read(JTAGIdCodeOnDLS());
	builder.block_until(BarrierOnFPGA(), Barrier::jtag);
	auto program = builder.done();
	stadls::vx::v3::run(connection, {program});

	if (dynamic_cast<JTAGIdCode const&>(jtag_id_ticket.get()).get_version() != 3) {
		std::stringstream ss;
		ss << "Unexpected chip version: ";
		ss << dynamic_cast<JTAGIdCode const&>(jtag_id_ticket.get()).get_version();
		throw std::runtime_error(ss.str());
	}
}

} // namespace

namespace grenade::vx::execution::backend {

InitializedConnection::InitializedConnection(
    hxcomm::vx::ConnectionVariant&& connection,
    std::vector<stadls::vx::v3::SystemInit> const& inits) :
    m_connection(std::make_unique<hxcomm::vx::ConnectionVariant>(std::move(connection))),
    m_expected_link_notification_count(halco::hicann_dls::vx::v3::PhyConfigFPGAOnDLS::size),
    m_init(*m_connection)
{
	log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("grenade.backend.InitializedConnection");
	using namespace stadls::vx;
	using namespace stadls::vx::v3;
	using namespace haldls::vx::v3;
	using namespace halco::hicann_dls::vx::v3;

	hate::Timer timer;

	std::vector<stadls::vx::v3::PlaybackProgram> init_programs;
	for (auto const& init : inits) {
		PlaybackProgramBuilder init_builder;
		// disable event recording, it is enabled only for realtime sections with event recording
		// request later
		{
			EventRecordingConfig config;
			config.set_enable_event_recording(false);
			init_builder.write(EventRecordingConfigOnFPGA(), config);
		}
		init_builder.merge_back(stadls::vx::generate(init).builder);
		init_programs.emplace_back(init_builder.done());
	}
	m_init.set(std::move(init_programs), std::nullopt, true);

	m_expected_link_notification_count = 0;

	perform_hardware_check(*m_connection);

	LOG4CXX_TRACE(logger, "InitializedConnection(): Initialized in " << timer.print() << ".");
}

InitializedConnection::InitializedConnection(hxcomm::vx::ConnectionVariant&& connection) :
    InitializedConnection(std::move(connection), [&connection]() {
	    std::vector<stadls::vx::v3::SystemInit> inits;
	    auto hwdb_entries = std::visit(
	        [](auto const& connection) { return connection.get_hwdb_entry(); }, connection);
	    for (auto const& hwdb_entry : hwdb_entries) {
		    inits.push_back(stadls::vx::v3::SystemInit(hwdb_entry));
	    }
	    return inits;
    }())
{
}

InitializedConnection::InitializedConnection() :
    InitializedConnection(hxcomm::vx::get_connection_from_env())
{
}

hxcomm::vx::ConnectionVariant& InitializedConnection::get_connection() const
{
	if (!m_connection) {
		throw std::logic_error("Unexpected access to moved-from object.");
	}
	return *m_connection;
}

std::vector<hxcomm::ConnectionTimeInfo> InitializedConnection::get_time_info() const
{
	return std::visit([](auto const& c) { return c.get_time_info(); }, get_connection());
}

std::vector<std::string> InitializedConnection::get_unique_identifier(
    std::optional<std::string> const& hwdb_path) const
{
	return std::visit(
	    [&hwdb_path](auto const& c) { return c.get_unique_identifier(hwdb_path); },
	    get_connection());
}

std::vector<std::string> InitializedConnection::get_bitfile_info() const
{
	return std::visit([](auto const& c) { return c.get_bitfile_info(); }, get_connection());
}

std::vector<std::string> InitializedConnection::get_remote_repo_state() const
{
	return std::visit([](auto const& c) { return c.get_remote_repo_state(); }, get_connection());
}

std::vector<hxcomm::HwdbEntry> InitializedConnection::get_hwdb_entry() const
{
	return std::visit([](auto const& c) { return c.get_hwdb_entry(); }, get_connection());
}

hxcomm::vx::ConnectionVariant&& InitializedConnection::release()
{
	return std::move(get_connection());
}

stadls::vx::v3::ReinitStackEntry InitializedConnection::create_reinit_stack_entry()
{
	return std::visit([](auto& c) { return stadls::vx::ReinitStackEntry(c); }, get_connection());
}

bool InitializedConnection::is_quiggeldy() const
{
	return std::holds_alternative<hxcomm::vx::QuiggeldyConnection>(get_connection());
}

std::vector<common::ChipOnConnection> InitializedConnection::get_chips_on_connection() const
{
	return {common::ChipOnConnection()};
}

size_t InitializedConnection::size() const
{
	return std::visit([](auto const& c) { return c.size(); }, get_connection());
}

} // namespace grenade::vx::execution::backend
