#include "grenade/vx/execution/detail/execution_instance_executor.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/backend/playback_program.h"
#include "grenade/vx/execution/detail/execution_instance_chip_snippet_config_visitor.h"
#include "grenade/vx/execution/detail/execution_instance_chip_snippet_ppu_usage_visitor.h"
#include "grenade/vx/execution/detail/generator/madc.h"
#include "grenade/vx/execution/detail/ppu_program_generator.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/detail/status.h"
#include "grenade/vx/signal_flow/execution_time_info.h"
#include "grenade/vx/signal_flow/output_data.h"
#include "grenade/vx/signal_flow/vertex/pad_readout.h"
#include "halco/hicann-dls/vx/v3/readout.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/padi.h"
#include "haldls/vx/v3/readout.h"
#include "hate/timer.h"
#include "hate/type_index.h"
#include "hate/type_traits.h"
#include "hate/variant.h"
#include "lola/vx/ppu.h"
#include "stadls/vx/constants.h"
#include "stadls/vx/v3/playback_program_builder.h"
#include <filesystem>
#include <iterator>
#include <boost/range/combine.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx::execution::detail {

signal_flow::OutputData ExecutionInstanceExecutor::PostProcessor::operator()(
    backend::PlaybackProgram const& playback_program)
{
	auto logger =
	    log4cxx::Logger::getLogger("grenade.ExecutionInstanceExecutor.Program.PostProcessor");

	auto realtime_results = realtime(playback_program);

	size_t const num_realtime_snippets = realtime_results.size();
	signal_flow::OutputData output_data;
	output_data.snippets.resize(num_realtime_snippets);

	for (size_t i = 0; auto& result : realtime_results) {
		output_data.snippets.at(i).merge(std::move(result.data));
		i++;
	}

	signal_flow::ExecutionTimeInfo execution_time_info;
	signal_flow::ExecutionHealthInfo::ExecutionInstance execution_health_info;
	for (auto const& [chip_on_connection, chip] : chips) {
		for (auto const& result : realtime_results) {
			execution_time_info
			    .realtime_duration_per_execution_instance[execution_instance][chip_on_connection] +=
			    result.total_realtime_duration.at(chip_on_connection);
		}

		// extract PPU read hooks
		output_data.read_ppu_symbols.resize(chip.ppu_read_hooks_results.size());
		for (size_t b = 0; b < chip.ppu_read_hooks_results.size(); ++b) {
			output_data.read_ppu_symbols.at(b)[execution_instance][chip_on_connection] =
			    chip.ppu_read_hooks_results.at(b).evaluate();
		}

		for (size_t i = 0; i < chip.health_info_results_pre.size(); ++i) {
			execution_health_info.chips[chip_on_connection] +=
			    (chip.health_info_results_post.at(i).get_execution_health_info() -=
			     chip.health_info_results_pre.at(i).get_execution_health_info());
		}

		for (size_t i = 0; i < output_data.snippets.size(); i++) {
			// add pre-execution config to result data map
			output_data.snippets.at(i).pre_execution_chips[execution_instance] =
			    playback_program.chips.at(chip_on_connection).chip_configs[i];
		}
	}

	output_data.execution_time_info = execution_time_info;

	// annotate execution health info
	signal_flow::ExecutionHealthInfo execution_health_info_total;
	execution_health_info_total.execution_instances[execution_instance] = execution_health_info;
	output_data.execution_health_info = execution_health_info_total;

	return output_data;
}


ExecutionInstanceExecutor::ExecutionInstanceExecutor(
    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
    signal_flow::InputData const& input_data,
    signal_flow::OutputData& output_data,
    std::vector<
        std::map<common::ChipOnConnection, std::reference_wrapper<lola::vx::v3::Chip const>>> const&
        configs,
    ExecutionInstanceHooks& hooks,
    std::vector<common::ChipOnConnection> const& chips_on_connection,
    grenade::common::ExecutionInstanceID const& execution_instance) :
    m_graphs(graphs),
    m_input_data(input_data),
    m_output_data(output_data),
    m_configs(configs),
    m_hooks(hooks),
    m_chips_on_connection(chips_on_connection),
    m_execution_instance(execution_instance)
{
}

std::pair<backend::PlaybackProgram, ExecutionInstanceExecutor::PostProcessor>
ExecutionInstanceExecutor::operator()() const
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceExecutor");

	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace lola::vx::v3;
	using namespace haldls::vx::v3;
	using namespace stadls::vx::v3;

	assert(
	    (std::set<size_t>{
	         m_output_data.snippets.size(), m_input_data.snippets.size(), m_graphs.size(),
	         m_configs.size()}
	         .size() == 1));
	size_t const snippet_count = m_output_data.snippets.size();

	hate::Timer const configs_timer;

	backend::PlaybackProgram playback_program;

	std::map<common::ChipOnConnection, ExecutionInstanceChipPPUProgramCompiler::Result>
	    ppu_programs;
	for (auto const& chip_on_connection : m_chips_on_connection) {
		bool const has_hook_around_realtime =
		    !m_hooks.chips.at(chip_on_connection).pre_realtime.empty() ||
		    !m_hooks.chips.at(chip_on_connection).inside_realtime_begin.empty() ||
		    !m_hooks.chips.at(chip_on_connection).inside_realtime_end.empty() ||
		    !m_hooks.chips.at(chip_on_connection).post_realtime.empty();

		playback_program.chips[chip_on_connection].has_hooks_around_realtime =
		    has_hook_around_realtime;
		playback_program.chips[chip_on_connection].pre_initial_config_hook =
		    std::move(m_hooks.chips.at(chip_on_connection).pre_static_config);

		for (size_t j = 0; j < snippet_count; j++) {
			playback_program.chips[chip_on_connection].chip_configs.push_back(
			    m_configs.at(j).at(chip_on_connection).get());

			ExecutionInstanceChipSnippetConfigVisitor(
			    m_graphs[j], chip_on_connection, m_execution_instance)(
			    playback_program.chips.at(chip_on_connection).chip_configs[j]);
		}

		auto const ppu_program = ExecutionInstanceChipPPUProgramCompiler(
		    m_graphs, m_input_data, m_hooks, chip_on_connection, m_execution_instance)();
		ppu_program.apply(playback_program);
		ppu_programs[chip_on_connection] = ppu_program;
	}

	auto [realtime_program, realtime_post_processor] = ExecutionInstanceRealtimeExecutor(
	    m_graphs, m_input_data.snippets, m_output_data.snippets, ppu_programs,
	    m_chips_on_connection, m_execution_instance)();

	PostProcessor post_processor;
	post_processor.execution_instance = m_execution_instance;
	post_processor.realtime = std::move(realtime_post_processor);

	size_t const batch_size = m_input_data.batch_size();

	// Experiment assembly
	std::vector<std::map<common::ChipOnConnection, PlaybackProgramBuilder>> assembled_builders(
	    batch_size);
	for (size_t i = 0; i < batch_size; i++) {
		for (auto const& chip_on_connection : m_chips_on_connection) {
			auto& local_realtime_program = realtime_program.chips.at(chip_on_connection);
			auto& local_hooks = m_hooks.chips.at(chip_on_connection);
			auto& local_playback_program = playback_program.chips.at(chip_on_connection);
			AbsoluteTimePlaybackProgramBuilder program_builder;
			haldls::vx::v3::Timer::Value config_time = realtime_program.snippets[0]
			                                               .at(chip_on_connection)
			                                               .realtimes[i]
			                                               .pre_realtime_duration;
			for (size_t j = 0; j < snippet_count; j++) {
				if (j > 0) {
					config_time += realtime_program.snippets[j - 1]
					                   .at(chip_on_connection)
					                   .realtimes[i]
					                   .realtime_duration;
					program_builder.write(
					    config_time, ChipOnDLS(), local_playback_program.chip_configs[j],
					    local_playback_program.chip_configs[j - 1]);
				}
				realtime_program.snippets[j].at(chip_on_connection).realtimes[i].builder +=
				    (config_time - realtime_program.snippets[j]
				                       .at(chip_on_connection)
				                       .realtimes[i]
				                       .pre_realtime_duration);
				program_builder.merge(
				    realtime_program.snippets[j].at(chip_on_connection).realtimes[i].builder);
			}

			// insert inside_realtime hook
			if (i < batch_size - 1) {
				program_builder.copy(local_hooks.inside_realtime);
			} else {
				program_builder.merge(local_hooks.inside_realtime);
			}

			// reset config to initial config at end of each realtime_row if experiment has multiple
			// configs
			if (realtime_program.snippets.size() > 1) {
				config_time += realtime_program.snippets[realtime_program.snippets.size() - 1]
				                   .at(chip_on_connection)
				                   .realtimes[i]
				                   .realtime_duration;
				if (i < batch_size - 1) {
					program_builder.write(
					    config_time, ChipOnDLS(), local_playback_program.chip_configs[0],
					    local_playback_program
					        .chip_configs[local_playback_program.chip_configs.size() - 1]);
				}
			}

			// assemble playback_program from arm_madc and program_builder and if applicable,
			// start_ppu, stop_ppu and the playback hooks
			PlaybackProgramBuilder assembled_builder;
			if (i == 0 && local_realtime_program.uses_madc) {
				assembled_builder.merge_back(generate(generator::MADCArm()).builder);
			}
			// for the first batch entry, append start_ppu, arm_madc and pre_realtime hook
			if (i == 0) {
				assembled_builder.merge_back(local_hooks.pre_realtime);
			}
			// append inside_realtime_begin hook
			if (i < batch_size - 1) {
				assembled_builder.copy_back(local_hooks.inside_realtime_begin);
			} else {
				assembled_builder.merge_back(local_hooks.inside_realtime_begin);
			}
			// append realtime section
			assembled_builder.merge_back(program_builder.done());
			// append inside_realtime_end hook
			if (i < batch_size - 1) {
				assembled_builder.copy_back(local_hooks.inside_realtime_end);
			} else {
				assembled_builder.merge_back(local_hooks.inside_realtime_end);
			}
			// append ppu_finish_builders
			for (size_t j = 0; j < snippet_count; j++) {
				assembled_builder.merge_back(realtime_program.snippets[j]
				                                 .at(chip_on_connection)
				                                 .realtimes[i]
				                                 .ppu_finish_builder);
			}
			// append PPU read hooks
			if (local_playback_program.ppu_symbols) {
				auto [ppu_read_hooks_builder, ppu_read_hooks_result] =
				    generate(generator::PPUReadHooks(
				        local_hooks.read_ppu_symbols, *local_playback_program.ppu_symbols));
				assembled_builder.merge_back(ppu_read_hooks_builder);
				post_processor.chips[chip_on_connection].ppu_read_hooks_results.push_back(
				    std::move(ppu_read_hooks_result));
			}
			// wait for response data
			assembled_builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
			// for the last batch entry, append post_realtime hook
			if (i == batch_size - 1) {
				assembled_builder.merge_back(local_hooks.post_realtime);
			}
			// append cadc_finazlize_builder for periodic_cadc data readout
			assembled_builder.merge_back(local_realtime_program.cadc_finalize_builders.at(i));
			// for the last batch entry, stop the PPU
			if (i == batch_size - 1 && local_playback_program.ppu_symbols) {
				assembled_builder.merge_back(
				    generate(generator::PPUStop(*local_playback_program.ppu_symbols)).builder);
			}
			// Implement inter_batch_entry_wait (ensures minimal waiting time in between batch
			// entries)
			if (m_input_data.inter_batch_entry_wait.contains(m_execution_instance)) {
				// disable internal event routing to silence network activity and state
				for (auto const crossbar_node_coord : iter_all<CrossbarNodeOnDLS>()) {
					assembled_builder.write(crossbar_node_coord, CrossbarNode::drop_all);
				}
				assembled_builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
				assembled_builder.block_until(
				    TimerOnDLS(),
				    config_time + m_input_data.inter_batch_entry_wait.at(m_execution_instance)
				                      .toTimerOnFPGAValue());
				// enable internal event routing for next batch entry
				for (auto const crossbar_node_coord : iter_all<CrossbarNodeOnDLS>()) {
					assembled_builder.write(
					    crossbar_node_coord,
					    local_playback_program.chip_configs[0].crossbar.nodes[crossbar_node_coord]);
				}
				assembled_builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
			}
			assembled_builders.at(i)[chip_on_connection] = std::move(assembled_builder);
		}
	}

	// chunk builders into maximally-sized builders, add pre & post measurements
	std::vector<std::map<common::ChipOnConnection, PlaybackProgramBuilder>>
	    chunked_assembled_builders;
	size_t const pre_size_to_fpga = generate(generator::HealthInfo()).builder.done().size_to_fpga();
	size_t const post_size_to_fpga = pre_size_to_fpga;
	std::map<common::ChipOnConnection, PlaybackProgramBuilder> chunked_assembled_builder;
	for (auto& assembled_builders_per_chip : assembled_builders) {
		if (std::any_of(
		        assembled_builders_per_chip.begin(), assembled_builders_per_chip.end(),
		        [&](auto& assembled_builder_per_chip) {
			        auto& [chip_on_connection, assembled_builder] = assembled_builder_per_chip;
			        return (chunked_assembled_builder[chip_on_connection].size_to_fpga() +
			                assembled_builder.size_to_fpga() + pre_size_to_fpga +
			                post_size_to_fpga) > stadls::vx::playback_memory_size_to_fpga;
		        })) {
			chunked_assembled_builders.emplace_back(std::move(chunked_assembled_builder));
		}
		for (auto& [chip_on_connection, assembled_builder] : assembled_builders_per_chip) {
			chunked_assembled_builder[chip_on_connection].merge_back(assembled_builder);
		}
	}
	if (!chunked_assembled_builder.empty()) {
		chunked_assembled_builders.emplace_back(std::move(chunked_assembled_builder));
	}

	// add pre and post measurements and construct finalized playback programs
	for (auto& chunked_assembled_builder_per_chip : chunked_assembled_builders) {
		for (auto& [chip_on_connection, chunked_assembled_builder] :
		     chunked_assembled_builder_per_chip) {
			PlaybackProgramBuilder final_builder;

			auto [health_info_builder_pre, health_info_result_pre] =
			    generate(generator::HealthInfo());
			post_processor.chips[chip_on_connection].health_info_results_pre.push_back(
			    health_info_result_pre);
			final_builder.merge_back(health_info_builder_pre.done());
			final_builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);

			final_builder.merge_back(chunked_assembled_builder);

			auto [health_info_builder_post, health_info_result_post] =
			    generate(generator::HealthInfo());
			post_processor.chips[chip_on_connection].health_info_results_post.push_back(
			    health_info_result_post);
			final_builder.merge_back(health_info_builder_post.done());
			final_builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
			playback_program.chips[chip_on_connection].programs.emplace_back(final_builder.done());
		}
	}

	return {std::move(playback_program), std::move(post_processor)};
}

} // namespace grenade::vx::execution::detail
