#include "grenade/vx/execution_instance_node.h"

#include "halco/hicann-dls/vx/v2/coordinates.h"
#include "haldls/vx/arq.h"
#include "haldls/vx/phy.h"
#include "hate/timer.h"
#include "stadls/vx/v2/playback_program_builder.h"
#include "stadls/vx/v2/run.h"
#include <sstream>
#include <log4cxx/logger.h>

namespace {

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
    std::mutex& continuous_chunked_program_execution_mutex) :
    data_map(data_map),
    input_data_map(input_data_map),
    graph(graph),
    execution_instance(execution_instance),
    chip_config(chip_config),
    connection(connection),
    continuous_chunked_program_execution_mutex(continuous_chunked_program_execution_mutex),
    logger(log4cxx::Logger::getLogger("grenade.ExecutionInstanceNode"))
{}

void ExecutionInstanceNode::operator()(tbb::flow::continue_msg)
{
	using namespace stadls::vx::v2;
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;

	ExecutionInstanceBuilder builder(
	    graph, execution_instance, input_data_map, data_map, chip_config);

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
			} catch (std::runtime_error const&) {
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
