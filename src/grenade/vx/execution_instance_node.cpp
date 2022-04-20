#include "grenade/vx/execution_instance_node.h"

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/backend/run.h"
#include "grenade/vx/execution_instance_config_visitor.h"
#include "grenade/vx/ppu/status.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/timer.h"
#include "hate/timer.h"
#include <log4cxx/logger.h>

namespace grenade::vx {

ExecutionInstanceNode::ExecutionInstanceNode(
    IODataMap& data_map,
    IODataMap const& input_data_map,
    Graph const& graph,
    coordinate::ExecutionInstance const& execution_instance,
    lola::vx::v3::Chip const& chip_config,
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
	using namespace stadls::vx::v3;
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;

	hate::Timer const initial_config_timer;
	lola::vx::v3::Chip initial_config = chip_config;
	auto const ppu_symbols =
	    std::get<1>(ExecutionInstanceConfigVisitor(graph, execution_instance, initial_config)());
	LOG4CXX_TRACE(
	    logger,
	    "operator(): Constructed initial configuration in " << initial_config_timer.print() << ".");

	ExecutionInstanceBuilder builder(
	    graph, execution_instance, input_data_map, data_map, ppu_symbols, playback_hooks);

	hate::Timer const preprocess_timer;
	builder.pre_process();
	LOG4CXX_TRACE(
	    logger, "operator(): Preprocessed local vertices in " << preprocess_timer.print() << ".");

	// build PlaybackProgram
	hate::Timer const build_timer;

	// add pre static config playback hook
	auto config_builder = std::move(playback_hooks.pre_static_config);

	// generate static configuration
	config_builder.write(ChipOnDLS(), initial_config);

	// wait for CapMem to settle
	config_builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
	config_builder.write(TimerOnDLS(), haldls::vx::v3::Timer());
	config_builder.block_until(
	    TimerOnDLS(), haldls::vx::v3::Timer::Value(
	                      100000 * haldls::vx::v3::Timer::Value::fpga_clock_cycles_per_us));

	// bring PPUs in running state
	if (ppu_symbols) {
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			haldls::vx::v3::PPUControlRegister ctrl;
			ctrl.set_inhibit_reset(true);
			config_builder.write(ppu.toPPUControlRegisterOnDLS(), ctrl);
		}
		auto const ppu_status_coord = ppu_symbols->at("status").coordinate.toMin();
		// wait for PPUs to be ready
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			using namespace haldls::vx::v3;
			PollingOmnibusBlockConfig config;
			config.set_address(PPUMemoryWord::addresses<PollingOmnibusBlockConfig::Address>(
			                       PPUMemoryWordOnDLS(ppu_status_coord, ppu))
			                       .at(0));
			config.set_target(
			    PollingOmnibusBlockConfig::Value(static_cast<uint32_t>(ppu::Status::idle)));
			config.set_mask(PollingOmnibusBlockConfig::Value(0xffffffff));
			config_builder.write(PollingOmnibusBlockConfigOnFPGA(), config);
			config_builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
			config_builder.block_until(PollingOmnibusBlockOnFPGA(), PollingOmnibusBlock());
		}
	}
	auto initial_config_program = config_builder.done();

	// build realtime programs
	auto program = builder.generate();
	LOG4CXX_TRACE(logger, "operator(): Built PlaybackPrograms in " << build_timer.print() << ".");

	if (program.realtime.size() > 1 && connection.is_quiggeldy() &&
	    program.has_hook_around_realtime) {
		LOG4CXX_WARN(
		    logger, "operator(): Connection uses quiggeldy and more than one playback programs "
		            "shall be executed back-to-back with a pre or post realtime hook. Their "
		            "contiguity can't be guaranteed.");
	}

	// execute
	hate::Timer const exec_timer;
	if (!program.realtime.empty() || !initial_config_program.empty()) {
		std::lock_guard lock(continuous_chunked_program_execution_mutex);
		auto initial_config_reinit = connection.create_reinit_stack_entry();
		initial_config_reinit.set(initial_config_program, true);
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
