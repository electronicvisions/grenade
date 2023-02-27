#include "grenade/vx/execution/backend/connection.h"

#include "grenade/vx/execution/backend/run.h"
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
	if (std::holds_alternative<hxcomm::vx::ZeroMockConnection>(connection)) {
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
	stadls::vx::v3::run(connection, builder.done());

	if (jtag_id_ticket.get().get_version() != 3) {
		std::stringstream ss;
		ss << "Unexpected chip version: ";
		ss << jtag_id_ticket.get().get_version();
		throw std::runtime_error(ss.str());
	}
}

} // namespace

namespace grenade::vx::execution::backend {

Connection::Connection(hxcomm::vx::ConnectionVariant&& connection, Init const& init) :
    m_connection(std::move(connection)),
    m_expected_link_notification_count(halco::hicann_dls::vx::v3::PhyConfigFPGAOnDLS::size),
    m_init(m_connection)
{
	log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("grenade.backend.Connection");
	using namespace stadls::vx;
	using namespace stadls::vx::v3;
	using namespace haldls::vx::v3;
	using namespace halco::hicann_dls::vx::v3;

	hate::Timer timer;

	PlaybackProgramBuilder init_builder;
	// disable event recording, it is enabled only for realtime sections with event recording
	// request later
	{
		EventRecordingConfig config;
		config.set_enable_event_recording(false);
		init_builder.write(EventRecordingConfigOnFPGA(), config);
	}
	init_builder.merge_back(
	    std::visit([](auto const& i) { return stadls::vx::generate(i).builder; }, init));
	m_init.set(init_builder.done(), std::nullopt, true);

	m_expected_link_notification_count = 0;

	perform_hardware_check(m_connection);

	LOG4CXX_TRACE(logger, "Connection(): Initialized in " << timer.print() << ".");
}

Connection::Connection(hxcomm::vx::ConnectionVariant&& connection) :
    Connection(std::move(connection), stadls::vx::v3::ExperimentInit())
{}

Connection::Connection() : Connection(hxcomm::vx::get_connection_from_env()) {}

hxcomm::ConnectionTimeInfo Connection::get_time_info() const
{
	return std::visit([](auto const& c) { return c.get_time_info(); }, m_connection);
}

std::string Connection::get_unique_identifier(std::optional<std::string> const& hwdb_path) const
{
	return std::visit(
	    [hwdb_path](auto const& c) { return c.get_unique_identifier(hwdb_path); }, m_connection);
}

std::string Connection::get_bitfile_info() const
{
	return std::visit([](auto const& c) { return c.get_bitfile_info(); }, m_connection);
}

std::string Connection::get_remote_repo_state() const
{
	return std::visit([](auto const& c) { return c.get_remote_repo_state(); }, m_connection);
}

hxcomm::vx::ConnectionVariant&& Connection::release()
{
	return std::move(m_connection);
}

stadls::vx::v3::ReinitStackEntry Connection::create_reinit_stack_entry()
{
	return std::visit([](auto& c) { return stadls::vx::ReinitStackEntry(c); }, m_connection);
}

bool Connection::is_quiggeldy() const
{
	return std::holds_alternative<hxcomm::vx::QuiggeldyConnection>(m_connection);
}

} // namespace grenade::vx::execution::backend
