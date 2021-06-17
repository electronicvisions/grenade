#include "grenade/vx/backend/connection.h"

#include "grenade/vx/backend/run.h"
#include "halco/hicann-dls/vx/v2/barrier.h"
#include "halco/hicann-dls/vx/v2/highspeed_link.h"
#include "halco/hicann-dls/vx/v2/jtag.h"
#include "haldls/vx/v2/barrier.h"
#include "haldls/vx/v2/jtag.h"
#include "hxcomm/vx/connection_from_env.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/v2/run.h"

namespace {

void perform_hardware_check(hxcomm::vx::ConnectionVariant& connection)
{
	if (std::holds_alternative<hxcomm::vx::ZeroMockConnection>(connection)) {
		return;
	}

	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;
	using namespace stadls::vx::v2;

	// perform hardware version check(s)
	PlaybackProgramBuilder builder;
	auto jtag_id_ticket = builder.read(JTAGIdCodeOnDLS());
	builder.block_until(BarrierOnFPGA(), Barrier::jtag);
	stadls::vx::v2::run(connection, builder.done());

	if (jtag_id_ticket.get().get_version() != 2) {
		std::stringstream ss;
		ss << "Unexpected chip version: ";
		ss << jtag_id_ticket.get().get_version();
		throw std::runtime_error(ss.str());
	}
}

} // namespace

namespace grenade::vx::backend {

Connection::Connection(hxcomm::vx::ConnectionVariant&& connection, Init const& init) :
    m_connection(std::move(connection)),
    m_expected_link_notification_count(halco::hicann_dls::vx::v2::PhyConfigFPGAOnDLS::size)
{
	using namespace stadls::vx;
	std::visit(
	    [&](auto const& i) { backend::run(*this, stadls::vx::generate(i).builder.done()); }, init);

	m_expected_link_notification_count = 0;

	perform_hardware_check(m_connection);
}

Connection::Connection(hxcomm::vx::ConnectionVariant&& connection) :
    Connection(std::move(connection), stadls::vx::v2::ExperimentInit())
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

hxcomm::vx::ConnectionVariant&& Connection::release()
{
	return std::move(m_connection);
}

} // namespace grenade::vx
