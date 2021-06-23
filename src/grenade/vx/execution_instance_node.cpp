#include "grenade/vx/execution_instance_node.h"

#include "halco/hicann-dls/vx/v2/coordinates.h"
#include "haldls/vx/arq.h"
#include "haldls/vx/phy.h"
#include "haldls/vx/v2/barrier.h"
#include "hate/timer.h"
#include "stadls/vx/v2/playback_program_builder.h"
#include "stadls/vx/v2/run.h"
#include <sstream>
#include <log4cxx/logger.h>

namespace {

void check_link_notifications(
    log4cxx::Logger* logger,
    stadls::vx::v2::PlaybackProgram::highspeed_link_notifications_type const& link_notifications,
    size_t n_expected_notifications)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;
	using namespace stadls::vx::v2;

	std::map<PhyStatusOnFPGA, HighspeedLinkNotification> notis_per_phy;
	for (auto const& noti : link_notifications) {
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
		LOG4CXX_ERROR(logger, "All configured highspeed links down at the end of the experiment.");
	}
}

template <typename Connection>
void perform_post_fail_analysis(
    log4cxx::Logger* logger,
    Connection& connection,
    stadls::vx::v2::PlaybackProgram const& dead_program)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;
	using namespace stadls::vx::v2;

	{
		std::stringstream ss;
		ss << "perform_post_fail_analysis(): Experiment failed, ";
		bool empty_notifications = dead_program.get_highspeed_link_notifications().empty();
		if (empty_notifications) {
			ss << "did not receive any highspeed link notifications.";
		} else {
			ss << "reporting received notifications:\n";
			for (auto const& noti : dead_program.get_highspeed_link_notifications()) {
				ss << noti << "\n";
			}
		}
		LOG4CXX_ERROR(logger, ss.str());
	}

	{
		// perform post-mortem reads
		PlaybackProgramBuilder builder;

		// perform stat readout at the end of the experiment
		auto ticket_arq = builder.read(HicannARQStatusOnFPGA());

		std::vector<PlaybackProgram::ContainerTicket<PhyStatus>> tickets_phy;
		for (auto coord : iter_all<PhyStatusOnFPGA>()) {
			tickets_phy.emplace_back(builder.read(coord));
		}

		builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
		run(connection, builder.done());

		std::stringstream ss;
		ss << "perform_post_fail_analysis(): Experiment failed, reading post-mortem status.";
		ss << ticket_arq.get() << "\n";
		for (auto ticket_phy : tickets_phy) {
			ss << ticket_phy.get() << "\n";
		}
		LOG4CXX_ERROR(logger, ss.str());
	}
}

} // namespace

namespace grenade::vx {

ExecutionInstanceNode::ExecutionInstanceNode(
    IODataMap& data_map,
    IODataMap const& input_data_map,
    Graph const& graph,
    coordinate::ExecutionInstance const& execution_instance,
    ChipConfig const& chip_config,
    hxcomm::vx::ConnectionVariant& connection,
    std::mutex& continuous_chunked_program_execution_mutex,
    ExecutionInstancePlaybackHooks& playback_hooks) :
    data_map(data_map),
    input_data_map(input_data_map),
    graph(graph),
    execution_instance(execution_instance),
    chip_config(chip_config),
    connection(connection),
    continuous_chunked_program_execution_mutex(continuous_chunked_program_execution_mutex),
    playback_hooks(playback_hooks),
    logger(log4cxx::Logger::getLogger("grenade.ExecutionInstanceNode"))
{}

void ExecutionInstanceNode::operator()(tbb::flow::continue_msg)
{
	using namespace stadls::vx::v2;
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;

	ExecutionInstanceBuilder builder(
	    graph, execution_instance, input_data_map, data_map, chip_config, playback_hooks);

	hate::Timer const preprocess_timer;
	builder.pre_process();
	LOG4CXX_TRACE(
	    logger, "operator(): Preprocessed local vertices in " << preprocess_timer.print() << ".");

	// build PlaybackProgram
	hate::Timer const build_timer;
	auto program = builder.generate();
	LOG4CXX_TRACE(logger, "operator(): Built PlaybackPrograms in " << build_timer.print() << ".");

	// execute
	hate::Timer const exec_timer;
	if (!program.empty()) {
		std::lock_guard lock(continuous_chunked_program_execution_mutex);
		for (auto& p : program) {
			try {
				stadls::vx::v2::run(connection, p);
				check_link_notifications(
				    logger, p.get_highspeed_link_notifications(), PhyStatusOnFPGA::size);
			} catch (std::runtime_error const&) {
				check_link_notifications(
				    logger, p.get_highspeed_link_notifications(), PhyStatusOnFPGA::size);
				// TODO: use specific exception for fisch run() fails, cf. task #3724
				perform_post_fail_analysis(logger, connection, p);
				throw;
			}
		}
	}
	LOG4CXX_TRACE(
	    logger, "operator(): Executed built PlaybackPrograms in " << exec_timer.print() << ".");

	// extract output data map
	hate::Timer const post_timer;
	auto result_data_map = builder.post_process();
	LOG4CXX_TRACE(logger, "operator(): Evaluated in " << post_timer.print() << ".");

	// merge local data map into global data map
	data_map.merge(result_data_map);
}

} // namespace grenade::vx
