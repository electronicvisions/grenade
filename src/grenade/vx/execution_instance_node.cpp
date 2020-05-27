#include "grenade/vx/execution_instance_node.h"

#include <log4cxx/logger.h>
#include "halco/hicann-dls/vx/v2/coordinates.h"
#include "hate/timer.h"
#include "stadls/vx/v2/playback_program_builder.h"
#include "stadls/vx/v2/run.h"

namespace grenade::vx {

ExecutionInstanceNode::ExecutionInstanceNode(
    DataMap& data_map,
    DataMap const& input_data_map,
    Graph const& graph,
    coordinate::ExecutionInstance const& execution_instance,
    ChipConfig const& chip_config,
    hxcomm::vx::ConnectionVariant& connection) :
    data_map(data_map),
    input_data_map(input_data_map),
    graph(graph),
    execution_instance(execution_instance),
    chip_config(chip_config),
    connection(connection),
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
	LOG4CXX_TRACE(logger, "operator(): Built PlaybackProgram in " << build_timer.print() << ".");

	// execute
	hate::Timer const exec_timer;
	if (!program.empty()) {
		stadls::vx::v2::run(connection, program);
	}
	LOG4CXX_TRACE(
	    logger, "operator(): Executed built PlaybackProgram in " << exec_timer.print() << ".");

	// extract output data map
	hate::Timer const post_timer;
	auto result_data_map = builder.post_process();
	LOG4CXX_TRACE(logger, "operator(): Evaluated in " << post_timer.print() << ".");

	// merge local data map into global data map
	data_map.merge(result_data_map);
}

} // namespace grenade::vx
