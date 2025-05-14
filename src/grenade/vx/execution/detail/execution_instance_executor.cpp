#include "grenade/vx/execution/detail/execution_instance_executor.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/execution/backend/playback_program.h"
#include "grenade/vx/execution/detail/execution_instance_snippet_config_visitor.h"
#include "grenade/vx/execution/detail/execution_instance_snippet_ppu_usage_visitor.h"
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

	signal_flow::OutputData output_data;

	signal_flow::ExecutionTimeInfo execution_time_info;
	for (auto const& result : realtime_results) {
		execution_time_info.realtime_duration_per_execution_instance[execution_instance] +=
		    result.total_realtime_duration;
	}

	output_data.execution_time_info = execution_time_info;

	for (auto& result : realtime_results) {
		output_data.snippets.emplace_back(std::move(result.data));
	}

	signal_flow::ExecutionHealthInfo::ExecutionInstance execution_health_info;
	for (size_t i = 0; i < health_info_results_pre.size(); ++i) {
		execution_health_info +=
		    (health_info_results_post.at(i).get_execution_health_info() -=
		     health_info_results_pre.at(i).get_execution_health_info());
	}

	// extract PPU read hooks
	output_data.read_ppu_symbols.resize(ppu_read_hooks_results.size());
	for (size_t b = 0; b < ppu_read_hooks_results.size(); ++b) {
		output_data.read_ppu_symbols.at(b)[execution_instance] =
		    ppu_read_hooks_results.at(b).evaluate();
	}

	// annotate execution health info
	signal_flow::ExecutionHealthInfo execution_health_info_total;
	execution_health_info_total.execution_instances[execution_instance] = execution_health_info;
	output_data.execution_health_info = execution_health_info_total;

	for (size_t i = 0; i < output_data.snippets.size(); i++) {
		// add pre-execution config to result data map
		output_data.snippets[i].pre_execution_chips[execution_instance] =
		    playback_program.chip_configs[i];
	}
	return output_data;
}


ExecutionInstanceExecutor::ExecutionInstanceExecutor(
    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
    signal_flow::InputData const& input_data,
    signal_flow::OutputData& output_data,
    std::vector<std::reference_wrapper<lola::vx::v3::Chip const>> const& configs,
    signal_flow::ExecutionInstanceHooks& hooks,
    grenade::common::ExecutionInstanceID const& execution_instance) :
    m_graphs(graphs),
    m_input_data(input_data),
    m_output_data(output_data),
    m_configs(configs),
    m_hooks(hooks),
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
	size_t const realtime_column_count = m_output_data.snippets.size();

	hate::Timer const configs_timer;

	bool const has_hook_around_realtime =
	    !m_hooks.pre_realtime.empty() || !m_hooks.inside_realtime_begin.empty() ||
	    !m_hooks.inside_realtime_end.empty() || !m_hooks.post_realtime.empty();

	backend::PlaybackProgram playback_program{
	    std::vector<Chip>(m_configs.begin(), m_configs.end()),
	    std::nullopt,
	    has_hook_around_realtime,
	    std::move(m_hooks.pre_static_config),
	    std::nullopt,
	    {}};

	for (size_t i = 0; i < realtime_column_count; i++) {
		ExecutionInstanceSnippetConfigVisitor(
		    m_graphs[i], m_execution_instance)(playback_program.chip_configs[i]);
	}

	auto const ppu_program = ExecutionInstancePPUProgramCompiler(
	    m_graphs, m_input_data, m_hooks, m_execution_instance)();
	ppu_program.apply(playback_program);

	auto [realtime_program, realtime_post_processor] = ExecutionInstanceRealtimeExecutor(
	    m_graphs, m_input_data.snippets, m_output_data.snippets, ppu_program,
	    m_execution_instance)();

	// storage for PPU read-hooks results to evaluate post-execution
	std::vector<generator::PPUReadHooks::Result> ppu_read_hooks_results;

	// Experiment assembly
	// TODO: move into separate method, change loops such that first all innermost builders are
	// constructed and then they are reduced to builders per batch and then these are merged into
	// programs. The merging into programs is also to be moved to another separate method.
	std::vector<PlaybackProgram> programs;
	PlaybackProgramBuilder final_builder;
	std::vector<generator::HealthInfo::Result> health_info_results_pre;
	std::vector<generator::HealthInfo::Result> health_info_results_post;
	for (size_t i = 0; i < realtime_program.realtime_columns[0].realtimes.size(); i++) {
		if (final_builder.empty()) {
			auto [health_info_builder_pre, health_info_result_pre] =
			    generate(generator::HealthInfo());
			health_info_results_pre.push_back(health_info_result_pre);
			final_builder.merge_back(health_info_builder_pre.done());
			final_builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
		}

		AbsoluteTimePlaybackProgramBuilder program_builder;
		haldls::vx::v3::Timer::Value config_time =
		    realtime_program.realtime_columns[0].realtimes[i].pre_realtime_duration;
		for (size_t j = 0; j < realtime_column_count; j++) {
			if (j > 0) {
				config_time +=
				    realtime_program.realtime_columns[j - 1].realtimes[i].realtime_duration;
				program_builder.write(
				    config_time, ChipOnDLS(), playback_program.chip_configs[j],
				    playback_program.chip_configs[j - 1]);
			}
			realtime_program.realtime_columns[j].realtimes[i].builder +=
			    (config_time -
			     realtime_program.realtime_columns[j].realtimes[i].pre_realtime_duration);
			program_builder.merge(realtime_program.realtime_columns[j].realtimes[i].builder);
		}

		// insert inside_realtime hook
		if (i < realtime_program.realtime_columns[0].realtimes.size() - 1) {
			program_builder.copy(m_hooks.inside_realtime);
		} else {
			program_builder.merge(m_hooks.inside_realtime);
		}

		// reset config to initial config at end of each realtime_row if experiment has multiple
		// configs
		if (realtime_program.realtime_columns.size() > 1) {
			config_time +=
			    realtime_program.realtime_columns[realtime_program.realtime_columns.size() - 1]
			        .realtimes[i]
			        .realtime_duration;
			if (i < realtime_program.realtime_columns[0].realtimes.size() - 1) {
				program_builder.write(
				    config_time, ChipOnDLS(), playback_program.chip_configs[0],
				    playback_program.chip_configs[playback_program.chip_configs.size() - 1]);
			}
		}

		// assemble playback_program from arm_madc and program_builder and if applicable, start_ppu,
		// stop_ppu and the playback hooks
		PlaybackProgramBuilder assemble_builder;
		if (i == 0 && realtime_program.uses_madc) {
			assemble_builder.merge_back(generate(generator::MADCArm()).builder);
		}
		// for the first batch entry, append start_ppu, arm_madc and pre_realtime hook
		if (i == 0) {
			assemble_builder.merge_back(m_hooks.pre_realtime);
		}
		// append inside_realtime_begin hook
		if (i < realtime_program.realtime_columns[0].realtimes.size() - 1) {
			assemble_builder.copy_back(m_hooks.inside_realtime_begin);
		} else {
			assemble_builder.merge_back(m_hooks.inside_realtime_begin);
		}
		// append realtime section
		assemble_builder.merge_back(std::move(program_builder.done()));
		// append inside_realtime_end hook
		if (i < realtime_program.realtime_columns[0].realtimes.size() - 1) {
			assemble_builder.copy_back(m_hooks.inside_realtime_end);
		} else {
			assemble_builder.merge_back(m_hooks.inside_realtime_end);
		}
		// append ppu_finish_builders
		for (size_t j = 0; j < realtime_column_count; j++) {
			assemble_builder.merge_back(
			    realtime_program.realtime_columns[j].realtimes[i].ppu_finish_builder);
		}
		// append PPU read hooks
		if (playback_program.ppu_symbols) {
			auto [ppu_read_hooks_builder, ppu_read_hooks_result] = generate(
			    generator::PPUReadHooks(m_hooks.read_ppu_symbols, *playback_program.ppu_symbols));
			assemble_builder.merge_back(ppu_read_hooks_builder);
			ppu_read_hooks_results.push_back(std::move(ppu_read_hooks_result));
		}
		// wait for response data
		assemble_builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
		// for the last batch entry, append post_realtime hook
		if (i == realtime_program.realtime_columns[0].realtimes.size() - 1) {
			assemble_builder.merge_back(m_hooks.post_realtime);
		}
		// append cadc_finazlize_builder for periodic_cadc data readout
		assemble_builder.merge_back(realtime_program.cadc_finalize_builders.at(i));
		// for the last batch entry, stop the PPU
		if (i == realtime_program.realtime_columns[0].realtimes.size() - 1 &&
		    playback_program.ppu_symbols) {
			assemble_builder.merge_back(
			    generate(generator::PPUStop(*playback_program.ppu_symbols)).builder);
		}
		// Implement inter_batch_entry_wait (ensures minimal waiting time in between batch entries)
		if (m_input_data.inter_batch_entry_wait.contains(m_execution_instance)) {
			assemble_builder.block_until(
			    TimerOnDLS(),
			    config_time + m_input_data.inter_batch_entry_wait.at(m_execution_instance)
			                      .toTimerOnFPGAValue());
		}
		// Create playback programs with the maximal amount of batch entries that fit in as a whole
		if ((final_builder.size_to_fpga() + assemble_builder.size_to_fpga() +
		     generate(generator::HealthInfo()).builder.done().size_to_fpga()) >
		    stadls::vx::playback_memory_size_to_fpga) {
			auto [health_info_builder_post, health_info_result_post] =
			    generate(generator::HealthInfo());
			health_info_results_post.push_back(health_info_result_post);
			final_builder.merge_back(health_info_builder_post.done());
			final_builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
			programs.push_back(std::move(final_builder.done()));
		}
		final_builder.merge_back(assemble_builder);
	}
	if (!final_builder.empty()) {
		auto [health_info_builder_post, health_info_result_post] =
		    generate(generator::HealthInfo());
		health_info_results_post.push_back(health_info_result_post);
		final_builder.merge_back(health_info_builder_post.done());
		final_builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
		programs.push_back(std::move(final_builder.done()));
	}
	playback_program.programs = std::move(programs);

	return {
	    std::move(playback_program),
	    PostProcessor{
	        m_execution_instance, ppu_read_hooks_results, health_info_results_pre,
	        health_info_results_post, std::move(realtime_post_processor)}};
}

} // namespace grenade::vx::execution::detail
