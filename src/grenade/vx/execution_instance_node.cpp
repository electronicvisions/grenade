#include "grenade/vx/execution_instance_node.h"

#include <log4cxx/logger.h>
#include "halco/hicann-dls/vx/coordinates.h"
#include "hate/timer.h"
#include "stadls/vx/playback_program_builder.h"
#include "stadls/vx/run.h"

namespace grenade::vx {

ExecutionInstanceNode::ExecutionInstanceNode(
    DataMap& data_map,
    ExecutionInstanceBuilder& builder,
    hxcomm::vx::ConnectionVariant& connection,
    std::vector<Graph::vertex_descriptor> vertices) :
    data_map(data_map),
    builder(builder),
    connection(connection),
    logger(log4cxx::Logger::getLogger("grenade.ExecutionInstanceNode")),
    vertices(vertices)
{}

void ExecutionInstanceNode::operator()(tbb::flow::continue_msg)
{
	using namespace stadls::vx;
	using namespace halco::common;
	using namespace halco::hicann_dls::vx;

	hate::Timer const preprocess_timer;
	for (auto const v : vertices) {
		builder.process(v);
	}
	LOG4CXX_TRACE(
	    logger, "operator(): Preprocessed local vertices in " << preprocess_timer.print() << ".");

	// build PlaybackProgram
	hate::Timer const build_timer;
	auto playback_program_builder = builder.generate();
	LOG4CXX_TRACE(logger, "operator(): Built PlaybackProgram in " << build_timer.print() << ".");

	// execute
	hate::Timer const exec_timer;
	if (!playback_program_builder.empty()) {
		auto program = playback_program_builder.done();
		stadls::vx::run(connection, program);
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
