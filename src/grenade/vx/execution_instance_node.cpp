#include "grenade/vx/execution_instance_node.h"

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/backend/run.h"
#include "hate/timer.h"
#include <log4cxx/logger.h>

namespace grenade::vx {

ExecutionInstanceNode::ExecutionInstanceNode(
    IODataMap& data_map,
    IODataMap const& input_data_map,
    Graph const& graph,
    coordinate::ExecutionInstance const& execution_instance,
    ChipConfig const& chip_config,
    backend::Connection& connection,
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
	if (!program.realtime.empty() || !program.static_config.empty()) {
		std::lock_guard lock(continuous_chunked_program_execution_mutex);
		auto static_config_reinit = connection.create_reinit_stack_entry();
		static_config_reinit.set(program.static_config, true);
		for (auto& p : program.realtime) {
			backend::run(connection, p);
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
