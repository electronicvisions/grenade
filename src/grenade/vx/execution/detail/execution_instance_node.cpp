#include "grenade/vx/execution/detail/execution_instance_node.h"

#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/execution/backend/initialized_connection.h"
#include "grenade/vx/execution/backend/playback_program.h"
#include "grenade/vx/execution/backend/stateful_connection.h"
#include "grenade/vx/execution/backend/stateful_connection_run.h"
#include "grenade/vx/execution/detail/execution_instance_executor.h"
#include "grenade/vx/execution/detail/generator/capmem.h"
#include "grenade/vx/execution/detail/generator/get_state.h"
#include "grenade/vx/execution/detail/generator/health_info.h"
#include "grenade/vx/execution/detail/generator/madc.h"
#include "grenade/vx/execution/detail/generator/ppu.h"
#include "grenade/vx/network/abstract/execution_instance_global.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/detail/status.h"
#include "grenade/vx/ppu/detail/stopped.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/omnibus_constants.h"
#include "haldls/vx/v3/timer.h"
#include "hate/timer.h"
#include <mutex>
#include <log4cxx/logger.h>

namespace grenade::vx::execution::detail {

ExecutionInstanceNode::ExecutionInstanceNode(
    std::vector<grenade::common::OutputData>& output_data,
    std::mutex& results_mutex,
    std::vector<std::shared_ptr<grenade::common::LinkedTopology>> const& topologies,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance,
    std::vector<std::reference_wrapper<grenade::common::InputData const>> const& input_data,
    backend::StatefulConnection& connection,
    ExecutionInstanceHooks& hooks,
    std::vector<grenade::common::VertexOnTopology> const& execution_instance_vertex_descriptors) :
    output_data(output_data),
    results_mutex(results_mutex),
    topologies(topologies),
    execution_instance(execution_instance),
    input_data(input_data),
    connection(connection),
    hooks(hooks),
    execution_instance_vertex_descriptors(execution_instance_vertex_descriptors),
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
	    topologies, execution_instance_vertex_descriptors, output_data, input_data, hooks,
	    connection.get_chips_on_connection());

	hate::Timer const compile_timer;

	std::map<common::ChipOnConnection, hxcomm::HwdbEntry> chip_hwdb_entries;
	auto chips_on_connection = connection.get_chips_on_connection();
	auto hwdb_entries = connection.get_hwdb_entry();
	assert(chips_on_connection.size() == hwdb_entries.size());

	for (size_t i = 0; i < chips_on_connection.size(); i++) {
		chip_hwdb_entries.emplace(chips_on_connection.at(i), std::move(hwdb_entries.at(i)));
	}

	auto [playback_program, post_processor] = executor(chip_hwdb_entries);

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

	auto local_results = post_processor(std::move(playback_program));

	// alter global results
	std::lock_guard lock(results_mutex);

	// add execution duration per hardware to result data map
	if (!local_results.at(local_results.size() - 1)
	         .execution_instances.contains(execution_instance)) {
		local_results.at(local_results.size() - 1)
		    .execution_instances.set(
		        execution_instance, network::abstract::ExecutionInstanceGlobal());
	}
	auto last_results_ptr = local_results.at(local_results.size() - 1)
	                            .execution_instances.get(execution_instance)
	                            .copy();
	auto& last_results =
	    dynamic_cast<network::abstract::ExecutionInstanceGlobal&>(*last_results_ptr);
	last_results.device_usage_duration = run_time_info.execution_duration;
	local_results.at(local_results.size() - 1)
	    .execution_instances.set(execution_instance, last_results);

	// merge local data map into global data map
	if (run_successful) {
		for (size_t i = 0; i < output_data.size(); ++i) {
			output_data.at(i).merge(local_results.at(i));
		}
	}

	// throw exception if run was not successful
	// typically another post-processing operation above will fail before this exception is
	// triggered
	if (!run_successful) {
		throw std::runtime_error("Run of playback program not successful.");
	}
}

} // namespace grenade::vx::execution::detail
