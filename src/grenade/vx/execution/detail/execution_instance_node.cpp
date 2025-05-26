#include "grenade/vx/execution/detail/execution_instance_node.h"

#include "grenade/vx/execution/backend/playback_program.h"
#include "grenade/vx/execution/backend/stateful_connection.h"
#include "grenade/vx/execution/backend/stateful_connection_run.h"
#include "grenade/vx/execution/detail/execution_instance_executor.h"
#include "hate/timer.h"
#include <log4cxx/logger.h>

namespace grenade::vx::execution::detail {

ExecutionInstanceNode::ExecutionInstanceNode(
    signal_flow::OutputData& data_maps,
    signal_flow::InputData const& input_data_maps,
    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
    grenade::common::ExecutionInstanceID const& execution_instance,
    grenade::common::ConnectionOnExecutor const& connection_on_executor,
    std::vector<
        std::map<common::ChipOnConnection, std::reference_wrapper<lola::vx::v3::Chip const>>> const&
        configs,
    backend::StatefulConnection& connection,
    ExecutionInstanceHooks& hooks) :
    data_maps(data_maps),
    input_data_maps(input_data_maps),
    graphs(graphs),
    execution_instance(execution_instance),
    connection_on_executor(connection_on_executor),
    configs(configs),
    connection(connection),
    hooks(hooks),
    logger(log4cxx::Logger::getLogger("grenade.ExecutionInstanceNode"))
{}

void ExecutionInstanceNode::operator()(tbb::flow::continue_msg)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	using namespace stadls::vx::v3;
	using namespace lola::vx::v3;

	ExecutionInstanceExecutor executor(
	    graphs, input_data_maps, data_maps, configs, hooks, connection.get_chips_on_connection(),
	    execution_instance);

	hate::Timer const compile_timer;

	auto [playback_program, post_processor] = executor();

	LOG4CXX_TRACE(
	    logger, "operator(): Compiled playback program in " << compile_timer.print() << ".");

	bool run_successful = true;
	backend::RunTimeInfo run_time_info;
	try {
		run_time_info = backend::run(connection, playback_program);
	} catch (std::runtime_error const& error) {
		LOG4CXX_ERROR(
		    logger, "operator(): Run of playback program not successful: " << error.what() << ".");
		run_successful = false;
	}

	auto output_data = post_processor(playback_program);

	// add execution duration per hardware to result data map
	assert(output_data.execution_time_info);
	output_data.execution_time_info->execution_duration_per_hardware[connection_on_executor] =
	    run_time_info.execution_duration;

	// merge local data map into global data map
	if (run_successful) {
		data_maps.merge(output_data);
	}

	// throw exception if run was not successful
	// typically another post-processing operation above will fail before this exception is
	// triggered
	if (!run_successful) {
		throw std::runtime_error("Run of playback program not successful.");
	}
}

} // namespace grenade::vx::execution::detail
