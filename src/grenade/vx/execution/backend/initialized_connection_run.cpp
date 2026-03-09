#include "grenade/vx/execution/backend/initialized_connection_run.h"

#include "grenade/vx/execution/backend/initialized_connection.h"
#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v3/barrier.h"
#include "halco/hicann-dls/vx/v3/fpga.h"
#include "halco/hicann-dls/vx/v3/highspeed_link.h"
#include "haldls/vx/arq.h"
#include "haldls/vx/fpga.h"
#include "haldls/vx/phy.h"
#include "haldls/vx/v3/barrier.h"
#include "stadls/vx/v3/container_ticket.h"
#include "stadls/vx/v3/playback_program.h"
#include "stadls/vx/v3/playback_program_builder.h"
#include "stadls/vx/v3/run.h"
#include <algorithm>
#include <sstream>
#include <vector>
#include <log4cxx/logger.h>

namespace {

void check_link_notifications(
    log4cxx::LoggerPtr logger,
    std::vector<stadls::vx::v3::PlaybackProgram::highspeed_link_notifications_type>
        link_notifications,
    size_t n_expected_notifications)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	using namespace stadls::vx::v3;

	for (size_t i = 0; i < link_notifications.size(); i++) {
		std::map<PhyStatusOnFPGA, HighspeedLinkNotification> notis_per_phy;
		for (auto const& noti : link_notifications[i]) {
			if (!((n_expected_notifications > 0) && noti.get_link_up() &&
			      (notis_per_phy.find(noti.get_phy()) == notis_per_phy.end()))) {
				// one "link up" message per phy is expected (when turned on after init),
				// everything else is not expected:
				LOG4CXX_WARN(logger, noti);
			}
			notis_per_phy[noti.get_phy()] = noti;
		}

		// FPGA will send one up notification per phy after a successful highspeed init;
		// if no HS init is performed and the links are stable there will be zero notis.
		if (!notis_per_phy.empty() && (notis_per_phy.size() < n_expected_notifications)) {
			LOG4CXX_ERROR(logger, "Not all configured highspeed links sent link notifications.");
		}

		if ((notis_per_phy.size() == PhyStatusOnFPGA::size) &&
		    (std::count_if(notis_per_phy.begin(), notis_per_phy.end(), [](auto const& item) {
			     return item.second.get_link_up() == true;
		     }) == 0)) {
			LOG4CXX_ERROR(
			    logger, "All configured highspeed links down at the end of the experiment.");
		}
	}
}

template <typename Connection>
void perform_post_fail_analysis(
    log4cxx::LoggerPtr logger,
    Connection& connection,
    std::vector<std::reference_wrapper<stadls::vx::v3::PlaybackProgram>> const& dead_programs)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	using namespace stadls::vx::v3;

	auto connection_ids =
	    std::visit([](auto const& c) { return c.get_unique_identifier(); }, connection);
	size_t connection_size = std::visit([](auto const& c) { return c.size(); }, connection);
	assert(dead_programs.size() == connection_size);
	{
		std::stringstream ss;
		for (size_t i = 0; i < connection_size; i++) {
			ss << connection_ids[i] << " perform_post_fail_analysis(): Experiment failed, ";
			bool empty_notifications =
			    dead_programs[i].get().get_highspeed_link_notifications().empty();
			if (empty_notifications) {
				ss << "did not receive any highspeed link notifications.";
			} else {
				ss << "reporting received notifications:\n";
				for (auto const& noti : dead_programs[i].get().get_highspeed_link_notifications()) {
					ss << noti << "\n";
				}
			}
		}
		LOG4CXX_ERROR(logger, ss.str());
	}

	{
		std::vector<ContainerTicket> tickets_arq;
		std::vector<std::vector<ContainerTicket>> tickets_phy;
		std::vector<stadls::vx::v3::PlaybackProgram> programs;
		std::vector<std::reference_wrapper<stadls::vx::v3::PlaybackProgram>> programs_wrapped;
		for (size_t i = 0; i < connection_size; i++) {
			// perform post-mortem reads
			PlaybackProgramBuilder builder;

			// reset instruction timeout, since we may have failed with non-default value
			builder.write(InstructionTimeoutConfigOnFPGA(), InstructionTimeoutConfig());

			// perform stat readout at the end of the experiment
			tickets_arq.emplace_back(builder.read(HicannARQStatusOnFPGA()));

			std::vector<ContainerTicket> ticket_phy;
			for (auto coord : iter_all<PhyStatusOnFPGA>()) {
				ticket_phy.emplace_back(builder.read(coord));
			}
			tickets_phy.emplace_back(ticket_phy);

			builder.block_until(BarrierOnFPGA(), Barrier::omnibus);

			auto program = builder.done();

			programs.push_back(program);
		}

		for (size_t i = 0; i < connection_size; i++) {
			programs_wrapped.push_back(programs.at(i));
		}

		stadls::vx::v3::run(connection, programs_wrapped);

		std::stringstream ss;
		for (size_t i = 0; i < connection_size; i++) {
			ss << connection_ids[i]
			   << " perform_post_fail_analysis(): Experiment failed, reading post-mortem status.";
			ss << tickets_arq[i].get() << "\n";
			for (auto ticket_phy : tickets_phy[i]) {
				ss << ticket_phy.get() << "\n";
			}
		}
		LOG4CXX_ERROR(logger, ss.str());
	}
}

} // namespace

namespace grenade::vx::execution::backend {

stadls::vx::RunTimeInfo run(
    InitializedConnection& connection,
    std::vector<std::reference_wrapper<stadls::vx::v3::PlaybackProgram>> const& programs)
{
	log4cxx::LoggerPtr const logger = log4cxx::Logger::getLogger("grenade.backend.run()");

	stadls::vx::RunTimeInfo ret;
	try {
		ret = stadls::vx::v3::run(connection.get_connection(), programs);
		check_link_notifications(
		    logger,
		    [&programs]() {
			    std::vector<stadls::vx::v3::PlaybackProgram::highspeed_link_notifications_type>
			        link_notifications;
			    for (auto const& program : programs) {
				    link_notifications.emplace_back(
				        program.get().get_highspeed_link_notifications());
			    }
			    return link_notifications;
		    }(),
		    connection.m_expected_link_notification_count);
	} catch (std::runtime_error const&) {
		check_link_notifications(
		    logger,
		    [&programs]() {
			    std::vector<stadls::vx::v3::PlaybackProgram::highspeed_link_notifications_type>
			        link_notifications;
			    for (auto const& program : programs) {
				    link_notifications.emplace_back(
				        program.get().get_highspeed_link_notifications());
			    }
			    return link_notifications;
		    }(),
		    connection.m_expected_link_notification_count);
		// TODO: use specific exception for fisch run() fails, cf. task #3724
		perform_post_fail_analysis(logger, connection.get_connection(), programs);
		throw;
	}
	return ret;
}

stadls::vx::RunTimeInfo run(
    InitializedConnection& connection, std::vector<stadls::vx::v3::PlaybackProgram>&& programs)
{
	std::vector<std::reference_wrapper<stadls::vx::v3::PlaybackProgram>> programs_wrapped;
	for (auto& program : programs) {
		programs_wrapped.push_back(program);
	}
	return run(connection, programs_wrapped);
}

} // namespace grenade::vx::execution::backend
