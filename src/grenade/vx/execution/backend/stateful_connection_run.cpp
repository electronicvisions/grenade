#include "grenade/vx/execution/backend/stateful_connection_run.h"

#include "grenade/vx/execution/backend/initialized_connection_run.h"
#include "grenade/vx/execution/backend/playback_program.h"
#include "grenade/vx/execution/backend/run_time_info.h"
#include "grenade/vx/execution/backend/stateful_connection.h"
#include "grenade/vx/execution/detail/generator/capmem.h"
#include "grenade/vx/execution/detail/generator/get_state.h"
#include "grenade/vx/execution/detail/generator/ppu.h"
#include "hate/timer.h"
#include <functional>
#include <mutex>
#include <ranges>
#include <stdexcept>

namespace grenade::vx::execution::backend {

RunTimeInfo run(StatefulConnection& connection, PlaybackProgram& program)
{
	log4cxx::LoggerPtr const logger = log4cxx::Logger::getLogger("grenade.backend.run()");

	auto const chips_on_connection = connection.get_chips_on_connection();

	// Playback programs run when outscheduled.
	hate::Timer const schedule_out_replacement_timer;
	std::vector<stadls::vx::v3::PlaybackProgram> schedule_out_replacement_programs;
	for (auto const& [_, local_program] : program.chips) {
		stadls::vx::v3::PlaybackProgramBuilder schedule_out_replacement_builder;
		if (local_program.ppu_symbols) {
			schedule_out_replacement_builder.merge_back(
			    stadls::vx::v3::generate(
			        execution::detail::generator::PPUStop(*local_program.ppu_symbols))
			        .builder);
			schedule_out_replacement_builder.merge_back(
			    stadls::vx::v3::generate(
			        execution::detail::generator::GetState{local_program.ppu_symbols.has_value()})
			        .builder);
		}
		schedule_out_replacement_programs.emplace_back(schedule_out_replacement_builder.done());
	}
	LOG4CXX_TRACE(
	    logger, "operator(): Generated playback program for schedule out replacement in "
	                << schedule_out_replacement_timer.print() << ".");

	if (connection.is_quiggeldy() && std::ranges::any_of(program.chips, [](auto const& pair) {
		    return pair.second.has_hooks_around_realtime && pair.second.programs.size() > 1;
	    })) {
		LOG4CXX_WARN(
		    logger, "Connection uses quiggeldy and more than one playback programs "
		            "shall be executed back-to-back with a pre or post realtime hook. Their "
		            "contiguity can't be guaranteed.");
	}

	std::vector<bool> const differential_config = connection.get_enable_differential_config();

	// Playback programs to get state.
	hate::Timer const read_timer;
	std::vector<stadls::vx::v3::PlaybackProgram> get_state_programs;
	std::map<grenade::vx::common::ChipOnConnection, execution::detail::generator::GetState::Result>
	    get_state_results;

	for (size_t i = 0; i < chips_on_connection.size(); i++) {
		auto const& chip = chips_on_connection.at(i);
		auto const& local_program = program.chips.at(chip);

		if (differential_config.at(i) && local_program.ppu_symbols) {
			auto get_state = stadls::vx::v3::generate(
			    execution::detail::generator::GetState(local_program.ppu_symbols.has_value()));
			get_state_programs.emplace_back(get_state.builder.done());
			get_state_results[chip] = std::move(get_state.result);
		} else {
			get_state_programs.emplace_back(stadls::vx::v3::PlaybackProgram());
		}
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
	if (std::ranges::any_of(differential_config, std::identity{})) {
		connection_lock.lock();
	}

	hate::Timer const initial_config_program_timer;
	std::vector<bool> is_fresh_connection_config;
	std::vector<bool> enforce_base;
	std::vector<bool> has_capmem_changes;
	std::vector<bool> nothing_changed;
	for (size_t i = 0; i < chips_on_connection.size(); i++) {
		auto const& chip = chips_on_connection.at(i);
		auto const& local_program = program.chips.at(chip);
		is_fresh_connection_config.emplace_back(connection.m_configs.at(chip).get_is_fresh());

		connection.m_configs.at(chip).set_system(local_program.system_configs[0], true);
		if (local_program.external_ppu_dram_memory_config) {
			connection.m_configs.at(chip).set_external_ppu_dram_memory(
			    local_program.external_ppu_dram_memory_config);
		}

		enforce_base.emplace_back(
		    !differential_config.at(i) || is_fresh_connection_config.at(i) ||
		    !local_program.pre_initial_config_hook.empty());

		has_capmem_changes.emplace_back(
		    enforce_base.at(i) || connection.m_configs.at(chip).get_differential_changes_capmem());

		nothing_changed.emplace_back(
		    differential_config.at(i) && !is_fresh_connection_config.at(i) &&
		    !connection.m_configs.at(chip).get_has_differential() &&
		    local_program.pre_initial_config_hook.empty());
	}

	std::vector<stadls::vx::PlaybackProgram> base_programs;
	std::vector<stadls::vx::PlaybackProgram> differential_programs;
	for (size_t i = 0; i < chips_on_connection.size(); i++) {
		auto const& chip = chips_on_connection.at(i);
		auto& local_program = program.chips.at(chip);

		stadls::vx::v3::PlaybackProgramBuilder base_builder;
		base_builder.merge_back(local_program.pre_initial_config_hook);

		stadls::vx::v3::PlaybackProgramBuilder differential_builder;
		if (!nothing_changed.at(i)) {
			base_builder.write(
			    detail::StatefulChipConfigBaseCoordinate(),
			    detail::StatefulChipConfigBase(connection.m_configs.at(chip)));
			if (connection.m_configs.at(chip).get_has_differential()) {
				differential_builder.write(
				    detail::StatefulChipConfigDifferentialCoordinate(),
				    detail::StatefulChipConfigDifferential(connection.m_configs.at(chip)));
			}
		}

		base_programs.emplace_back(base_builder.done());
		differential_programs.emplace_back(differential_builder.done());
	}
	LOG4CXX_TRACE(
	    logger, "Generated playback program of initial config after "
	                << initial_config_program_timer.print() << ".");

	std::vector<stadls::vx::v3::PlaybackProgram> trigger_programs;
	for (auto const& [_, local_program] : program.chips) {
		trigger_programs.emplace_back(
		    local_program.ppu_symbols
		        ? stadls::vx::v3::generate(
		              execution::detail::generator::PPUStart(
		                  std::get<halco::hicann_dls::vx::v3::PPUMemoryBlockOnPPU>(
		                      local_program.ppu_symbols->at("status").coordinate)
		                      .toMin()))
		              .builder.done()
		        : stadls::vx::v3::PlaybackProgram());
	}

	// execute
	hate::Timer const exec_timer;
	std::vector<std::chrono::nanoseconds> connection_execution_duration_before(
	    chips_on_connection.size(), std::chrono::nanoseconds{0});
	std::vector<std::chrono::nanoseconds> connection_execution_duration_after(
	    chips_on_connection.size(), std::chrono::nanoseconds{0});

	bool runs_successful = true;


	// Execution of base programs and realtime snippets
	if (!std::ranges::all_of(
	        program.chips,
	        [](auto const& pair) {
		        return pair.second.programs.empty();
	        }) || // TO-DO: With the assert of same lenght for all, either all are empty or none???
	    !std::ranges::all_of(
	        base_programs, [](auto const& base_program) { return base_program.empty(); })) {
		// Reorder program vectors from (Chips, Snippets) to (Snippets, Chips)
		size_t max_realtime_section_length =
		    std::ranges::max_element(program.chips, {}, [](auto const& pair) {
			    return pair.second.programs.size();
		    })->second.programs.size();
		assert(std::ranges::all_of(program.chips, [max_realtime_section_length](auto const& pair) {
			return pair.second.programs.size() == max_realtime_section_length;
		}));

		std::vector<std::vector<std::reference_wrapper<stadls::vx::v3::PlaybackProgram>>>
		    realtime_sections(max_realtime_section_length);
		for (size_t chip_index = 0; chip_index < chips_on_connection.size(); chip_index++) {
			auto& local_program = program.chips.at(chips_on_connection.at(chip_index));
			for (size_t section_index = 0; section_index < local_program.programs.size();
			     section_index++) {
				realtime_sections.at(section_index)
				    .push_back(local_program.programs.at(section_index));
			}
		}

		// only lock execution section for non-differential config mode
		if (std::ranges::none_of(differential_config, std::identity{})) {
			connection_lock.lock();
		}

		// we are locked here in any case
		std::ranges::transform(
		    connection.get_time_info(), connection_execution_duration_before.begin(),
		    [](auto const& chip_time_info) { return chip_time_info.execution_duration; });

		// Only if something changed (re-)set base and differential reinit
		if (!std::ranges::all_of(nothing_changed, std::identity{})) {
			connection.m_reinit_base.set(
			    std::move(base_programs), std::nullopt,
			    std::ranges::any_of(enforce_base, std::identity{}));

			// Only enforce when not empty to support non-differential mode.
			// In differential mode it is always enforced on changes.
			connection.m_reinit_differential.set(
			    std::move(differential_programs), std::nullopt,
			    !std::ranges::all_of(differential_programs, [](auto const& differential_program) {
				    return differential_program.empty();
			    }));
		}

		// Never enforce, since the reinit is only filled after a schedule-out operation.
		std::vector<stadls::vx::v3::PlaybackProgram> schedule_in_programs(
		    chips_on_connection.size(), stadls::vx::v3::PlaybackProgram());
		connection.m_reinit_schedule_out_replacement.set(
		    std::move(schedule_in_programs), std::move(schedule_out_replacement_programs), false);

		// Always write capmem settling wait reinit, but only enforce it when the wait is
		// immediately required, i.e. after changes to the capmem.
		std::vector<stadls::vx::v3::PlaybackProgram> capmem_settling_waits(
		    chips_on_connection.size(),
		    stadls::vx::v3::generate(execution::detail::generator::CapMemSettlingWait())
		        .builder.done());
		connection.m_reinit_capmem_settling_wait.set(
		    std::move(capmem_settling_waits), std::nullopt,
		    std::ranges::any_of(has_capmem_changes, std::identity{}));

		// Always write (PPU) trigger reinit and enforce when not empty, i.e. when PPUs are
		// used.
		connection.m_reinit_start_ppus.set(
		    std::move(trigger_programs), std::nullopt,
		    !std::ranges::all_of(trigger_programs, [](auto const& trigger_program) {
			    return trigger_program.empty();
		    }));

		// Execute realtime sections
		for (auto& real_time_section : realtime_sections) {
			try {
				run(connection.m_initialized_connection, real_time_section);
			} catch (std::runtime_error const& error) {
				LOG4CXX_ERROR(
				    logger, "Run of playback program not successful: " << error.what() << ".");
				runs_successful = false;
			}
		}

		// If the PPUs (can) alter state, read it back to update current_config accordingly to
		// represent the actual hardware state.
		if (!std::ranges::all_of(get_state_programs, [](auto const& get_state_program) {
			    return get_state_program.empty();
		    })) {
			try {
				std::vector<std::reference_wrapper<stadls::vx::v3::PlaybackProgram>>
				    get_state_programs_wrapped;
				for (auto& get_state_program : get_state_programs) {
					get_state_programs_wrapped.push_back(get_state_program);
				}
				run(connection.m_initialized_connection, get_state_programs_wrapped);
			} catch (std::runtime_error const& error) {
				LOG4CXX_ERROR(
				    logger, "Run of playback program not successful: " << error.what() << ".");
				runs_successful = false;
			}
		}

		// unlock execution section for non-differential config mode
		if (std::ranges::none_of(differential_config, std::identity{})) {
			std::ranges::transform(
			    connection.get_time_info(), connection_execution_duration_after.begin(),
			    [](auto const& chip_time_info) { return chip_time_info.execution_duration; });
			connection_lock.unlock();
		}
	}
	LOG4CXX_TRACE(logger, "Executed built PlaybackPrograms in " << exec_timer.print() << ".");


	// update connection state
	hate::Timer const update_state_timer;
	for (size_t i = 0; i < chips_on_connection.size(); i++) {
		auto const& chip = chips_on_connection.at(i);
		auto const& local_program = program.chips.at(chip);

		if (local_program.system_configs.size() > 1 || local_program.ppu_symbols) {
			auto current_config =
			    local_program.system_configs[local_program.system_configs.size() - 1];
			if (get_state_results.contains(chip)) {
				get_state_results.at(chip).apply(std::visit(
				    [](auto& system) -> lola::vx::v3::Chip& { return system.chip; },
				    current_config));
			}
			if (runs_successful) {
				connection.m_configs.at(chip).set_system(current_config, false);
			} else {
				// reset connection state storage since no assumptions can be made after failure
				// TODO: create reset mechanism in class

				connection.m_configs.at(chip).reset();
				connection.m_configs.at(chip).set_enable_differential_config(
				    differential_config.at(i));
			}
		}
	}
	LOG4CXX_TRACE(logger, "Updated connection state in " << update_state_timer.print() << ".");

	// unlock execution section for differential config mode
	if (std::ranges::any_of(differential_config, std::identity{})) {
		if (!std::ranges::all_of(
		        program.chips, [](auto const& pair) { return pair.second.programs.empty(); }) ||
		    !std::ranges::all_of(
		        base_programs, [](auto const& base_program) { return base_program.empty(); })) {
			std::ranges::transform(
			    connection.get_time_info(), connection_execution_duration_after.begin(),
			    [](auto const& chip_time_info) { return chip_time_info.execution_duration; });
		}
		connection_lock.unlock();
	}

	// throw exception if any run was not successful
	if (!runs_successful) {
		throw std::runtime_error("At least one playback program execution was not successful.");
	}

	RunTimeInfo ret;
	for (size_t i = 0; i < chips_on_connection.size(); i++) {
		ret.execution_duration[chips_on_connection.at(i)] =
		    connection_execution_duration_after.at(i) - connection_execution_duration_before.at(i);
	}

	return ret;
}

RunTimeInfo run(StatefulConnection& connection, PlaybackProgram&& program)
{
	return run(connection, program);
}

} // namespace grenade::vx::execution::backend
