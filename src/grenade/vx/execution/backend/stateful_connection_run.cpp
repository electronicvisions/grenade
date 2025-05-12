#include "grenade/vx/execution/backend/stateful_connection_run.h"

#include "grenade/vx/execution/backend/initialized_connection_run.h"
#include "grenade/vx/execution/backend/playback_program.h"
#include "grenade/vx/execution/backend/run_time_info.h"
#include "grenade/vx/execution/backend/stateful_connection.h"
#include "grenade/vx/execution/detail/generator/capmem.h"
#include "grenade/vx/execution/detail/generator/get_state.h"
#include "grenade/vx/execution/detail/generator/ppu.h"
#include "hate/timer.h"
#include <mutex>

namespace grenade::vx::execution::backend {

RunTimeInfo run(StatefulConnection& connection, PlaybackProgram& program)
{
	log4cxx::LoggerPtr const logger = log4cxx::Logger::getLogger("grenade.backend.run()");

	hate::Timer const schedule_out_replacement_timer;
	stadls::vx::v3::PlaybackProgramBuilder schedule_out_replacement_builder;
	if (program.ppu_symbols) {
		schedule_out_replacement_builder.merge_back(
		    stadls::vx::v3::generate(execution::detail::generator::PPUStop(*program.ppu_symbols))
		        .builder);
		schedule_out_replacement_builder.merge_back(
		    stadls::vx::v3::generate(
		        execution::detail::generator::GetState{program.ppu_symbols.has_value()})
		        .builder);
	}
	auto schedule_out_replacement_program = schedule_out_replacement_builder.done();
	LOG4CXX_TRACE(
	    logger, "operator(): Generated playback program for schedule out replacement in "
	                << schedule_out_replacement_timer.print() << ".");

	if (program.programs.size() > 1 && connection.is_quiggeldy() &&
	    program.has_hooks_around_realtime) {
		LOG4CXX_WARN(
		    logger, "Connection uses quiggeldy and more than one playback programs "
		            "shall be executed back-to-back with a pre or post realtime hook. Their "
		            "contiguity can't be guaranteed.");
	}

	hate::Timer const read_timer;
	stadls::vx::v3::PlaybackProgram get_state_program;
	std::optional<execution::detail::generator::GetState::Result> get_state_result;
	if (connection.get_enable_differential_config() && program.ppu_symbols) {
		auto get_state = stadls::vx::v3::generate(
		    execution::detail::generator::GetState(program.ppu_symbols.has_value()));
		get_state_program = get_state.builder.done();
		get_state_result = get_state.result;
	}
	LOG4CXX_TRACE(
	    logger, "Generated playback program for read-back of PPU alterations in "
	                << read_timer.print() << ".");

	if (!connection.m_mutex) {
		throw std::logic_error("Unexpected access to moved-from object.");
	}
	std::unique_lock<std::mutex> connection_lock(*connection.m_mutex, std::defer_lock);
	// exclusive access to connection.m_state_storage and initialized connection required from here
	// in differential mode
	if (connection.get_enable_differential_config()) {
		connection_lock.lock();
	}

	hate::Timer const initial_config_program_timer;
	bool const is_fresh_connection_config = connection.m_config.get_is_fresh();
	connection.m_config.set_chip(program.chip_configs[0], true);
	if (program.external_ppu_dram_memory_config) {
		connection.m_config.set_external_ppu_dram_memory(program.external_ppu_dram_memory_config);
	}

	bool const enforce_base = !connection.get_enable_differential_config() ||
	                          is_fresh_connection_config ||
	                          !program.pre_initial_config_hook.empty();
	bool const has_capmem_changes =
	    enforce_base || connection.m_config.get_differential_changes_capmem();
	bool const nothing_changed =
	    connection.get_enable_differential_config() && !is_fresh_connection_config &&
	    !connection.m_config.get_has_differential() && program.pre_initial_config_hook.empty();

	stadls::vx::v3::PlaybackProgramBuilder base_builder;

	base_builder.merge_back(program.pre_initial_config_hook);

	stadls::vx::v3::PlaybackProgramBuilder differential_builder;
	if (!nothing_changed) {
		base_builder.write(
		    detail::StatefulConnectionConfigBaseCoordinate(),
		    detail::StatefulConnectionConfigBase(connection.m_config));
		if (connection.m_config.get_has_differential()) {
			differential_builder.write(
			    detail::StatefulConnectionConfigDifferentialCoordinate(),
			    detail::StatefulConnectionConfigDifferential(connection.m_config));
		}
	}

	auto base_program = base_builder.done();
	auto differential_program = differential_builder.done();
	LOG4CXX_TRACE(
	    logger, "Generated playback program of initial config after "
	                << initial_config_program_timer.print() << ".");

	auto const trigger_program =
	    program.ppu_symbols
	        ? stadls::vx::v3::generate(execution::detail::generator::PPUStart(
	                                       std::get<halco::hicann_dls::vx::v3::PPUMemoryBlockOnPPU>(
	                                           program.ppu_symbols->at("status").coordinate)
	                                           .toMin()))
	              .builder.done()
	        : stadls::vx::v3::PlaybackProgram();

	// execute
	hate::Timer const exec_timer;
	std::chrono::nanoseconds connection_execution_duration_before{0};
	std::chrono::nanoseconds connection_execution_duration_after{0};

	bool runs_successful = true;

	if (!program.programs.empty() || !base_program.empty()) {
		// only lock execution section for non-differential config mode
		if (!connection.get_enable_differential_config()) {
			connection_lock.lock();
		}

		// we are locked here in any case
		connection_execution_duration_before = connection.get_time_info().execution_duration;

		// Only if something changed (re-)set base and differential reinit
		if (!nothing_changed) {
			connection.m_reinit_base.set(base_program, std::nullopt, enforce_base);

			// Only enforce when not empty to support non-differential mode.
			// In differential mode it is always enforced on changes.
			connection.m_reinit_differential.set(
			    differential_program, std::nullopt, !differential_program.empty());
		}

		// Never enforce, since the reinit is only filled after a schedule-out operation.
		connection.m_reinit_schedule_out_replacement.set(
		    stadls::vx::v3::PlaybackProgram(), schedule_out_replacement_program, false);

		// Always write capmem settling wait reinit, but only enforce it when the wait is
		// immediately required, i.e. after changes to the capmem.
		connection.m_reinit_capmem_settling_wait.set(
		    stadls::vx::v3::generate(execution::detail::generator::CapMemSettlingWait())
		        .builder.done(),
		    std::nullopt, has_capmem_changes);

		// Always write (PPU) trigger reinit and enforce when not empty, i.e. when PPUs are used.
		connection.m_reinit_start_ppus.set(trigger_program, std::nullopt, !trigger_program.empty());

		// Execute realtime sections
		for (auto& p : program.programs) {
			try {
				run(connection.m_initialized_connection, p);
			} catch (std::runtime_error const& error) {
				LOG4CXX_ERROR(
				    logger, "Run of playback program not successful: " << error.what() << ".");
				runs_successful = false;
			}
		}

		// If the PPUs (can) alter state, read it back to update current_config accordingly to
		// represent the actual hardware state.
		if (!get_state_program.empty()) {
			try {
				run(connection.m_initialized_connection, get_state_program);
			} catch (std::runtime_error const& error) {
				LOG4CXX_ERROR(
				    logger, "Run of playback program not successful: " << error.what() << ".");
				runs_successful = false;
			}
		}

		// unlock execution section for non-differential config mode
		if (!connection.get_enable_differential_config()) {
			connection_execution_duration_after = connection.get_time_info().execution_duration;
			connection_lock.unlock();
		}
	}
	LOG4CXX_TRACE(logger, "Executed built PlaybackPrograms in " << exec_timer.print() << ".");

	// update connection state
	if (program.chip_configs.size() > 1 || program.ppu_symbols) {
		hate::Timer const update_state_timer;
		auto current_config = program.chip_configs[program.chip_configs.size() - 1];
		if (get_state_result) {
			get_state_result->apply(current_config);
		}
		if (runs_successful) {
			connection.m_config.set_chip(current_config, false);
		} else {
			// reset connection state storage since no assumptions can be made after failure
			// TODO: create reset mechanism in class
			bool const enable_differential_config = connection.get_enable_differential_config();
			connection.m_config = detail::StatefulConnectionConfig();
			connection.m_config.set_enable_differential_config(enable_differential_config);
		}
		LOG4CXX_TRACE(logger, "Updated connection state in " << update_state_timer.print() << ".");
	}

	// unlock execution section for differential config mode
	if (connection.get_enable_differential_config()) {
		if (!program.programs.empty() || !base_program.empty()) {
			connection_execution_duration_after = connection.get_time_info().execution_duration;
		}
		connection_lock.unlock();
	}

	// throw exception if any run was not successful
	if (!runs_successful) {
		throw std::runtime_error("At least one playback program execution was not successful.");
	}

	return RunTimeInfo{connection_execution_duration_after - connection_execution_duration_before};
}

RunTimeInfo run(StatefulConnection& connection, PlaybackProgram&& program)
{
	return run(connection, program);
}

} // namespace grenade::vx::execution::backend
