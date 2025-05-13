#include "grenade/vx/execution/detail/execution_instance_realtime_executor.h"

#include "grenade/vx/execution/backend/playback_program.h"
#include "grenade/vx/execution/detail/execution_instance_snippet_ppu_usage_visitor.h"
#include "grenade/vx/execution/detail/generator/madc.h"
#include "grenade/vx/execution/detail/ppu_program_generator.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/detail/status.h"
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

std::vector<ExecutionInstanceSnippetRealtimeExecutor::Result>
ExecutionInstanceRealtimeExecutor::PostProcessor::operator()(
    backend::PlaybackProgram const& playback_program)
{
	auto logger =
	    log4cxx::Logger::getLogger("grenade.ExecutionInstanceRealtimeExecutor.PostProcessor");

	std::vector<ExecutionInstanceSnippetRealtimeExecutor::Result> results;
	for (size_t i = 0; i < snippet_executors.size(); i++) {
		// extract output data map
		hate::Timer const post_timer;
		ExecutionInstanceSnippetRealtimeExecutor::PostProcessable post_processable;
		post_processable.realtime = playback_program.programs;
		post_processable.cadc_readout_tickets = cadc_readout_tickets;
		post_processable.periodic_cadc_readout_times = cadc_readout_time_information[i];
		auto result = snippet_executors[i].post_process(post_processable);
		results.push_back(std::move(result));
		LOG4CXX_TRACE(logger, "operator(): Evaluated in " << post_timer.print() << ".");
	}
	return results;
}


ExecutionInstanceRealtimeExecutor::ExecutionInstanceRealtimeExecutor(
    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
    std::vector<signal_flow::InputDataSnippet> const& input_data,
    std::vector<signal_flow::OutputDataSnippet>& output_data,
    ExecutionInstancePPUProgramCompiler::Result const& ppu_program,
    grenade::common::ExecutionInstanceID const& execution_instance) :
    m_graphs(graphs),
    m_input_data(input_data),
    m_output_data(output_data),
    m_ppu_program(ppu_program),
    m_execution_instance(execution_instance)
{
}

std::pair<
    ExecutionInstanceRealtimeExecutor::Program,
    ExecutionInstanceRealtimeExecutor::PostProcessor>
ExecutionInstanceRealtimeExecutor::operator()() const
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceRealtimeExecutor");

	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace lola::vx::v3;
	using namespace haldls::vx::v3;
	using namespace stadls::vx::v3;

	assert(
	    (std::set<size_t>{m_output_data.size(), m_input_data.size(), m_graphs.size()}.size() == 1));
	size_t const realtime_column_count = m_output_data.size();

	// vectors for storing the information on what chip components are used in the realtime snippets
	// before and after the current one (the according index) Example: usages_before[0] holds the
	// usages needed before the first realtime snippet etc...
	std::vector<ExecutionInstanceSnippetRealtimeExecutor::Usages> usages_before(
	    realtime_column_count);
	std::vector<ExecutionInstanceSnippetRealtimeExecutor::Usages> usages_after(
	    realtime_column_count);
	usages_before[0] = ExecutionInstanceSnippetRealtimeExecutor::Usages{
	    .madc_recording = false,
	    .event_recording = false,
	    .cadc_recording = typed_array<
	        std::set<signal_flow::vertex::CADCMembraneReadoutView::Mode>, HemisphereOnDLS>{{}}};
	usages_after[usages_after.size() - 1] = ExecutionInstanceSnippetRealtimeExecutor::Usages{
	    .madc_recording = false,
	    .event_recording = false,
	    .cadc_recording = typed_array<
	        std::set<signal_flow::vertex::CADCMembraneReadoutView::Mode>, HemisphereOnDLS>{{}}};

	// vector for storing all execution_instance_snippet_realtime_executors
	std::vector<ExecutionInstanceSnippetRealtimeExecutor> builders;
	std::vector<ExecutionInstanceSnippetRealtimeExecutor::Ret> realtime_columns;

	std::vector<bool> periodic_cadc_recording;
	std::vector<bool> periodic_cadc_dram_recording;
	bool uses_top_cadc = false;
	bool uses_bot_cadc = false;
	bool uses_madc = false;
	for (size_t i = 0; i < realtime_column_count; i++) {
		builders.emplace_back(
		    m_graphs[i], m_execution_instance, m_input_data[i], m_output_data[i],
		    m_ppu_program.symbols, i,
		    m_ppu_program.plasticity_rule_timed_recording_start_periods.at(i));

		hate::Timer const realtime_preprocess_timer;
		ExecutionInstanceSnippetRealtimeExecutor::Usages usages = builders[i].pre_process();
		if (usages.madc_recording) {
			uses_madc = true;
		}
		if (i < usages_before.size() - 1) {
			usages_before[i + 1] = usages;
		}
		if (i > 0) {
			usages_after[i - 1] = usages;
		}
		periodic_cadc_recording.push_back(std::any_of(
		    usages.cadc_recording.begin(), usages.cadc_recording.end(),
		    [](std::set<signal_flow::vertex::CADCMembraneReadoutView::Mode> const& mode) {
			    return mode.contains(signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic);
		    }));
		periodic_cadc_dram_recording.push_back(std::any_of(
		    usages.cadc_recording.begin(), usages.cadc_recording.end(),
		    [](std::set<signal_flow::vertex::CADCMembraneReadoutView::Mode> const& mode) {
			    return mode.contains(
			        signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic_on_dram);
		    }));
		uses_top_cadc = uses_top_cadc || !usages.cadc_recording[HemisphereOnDLS::top].empty();
		uses_bot_cadc = uses_bot_cadc || !usages.cadc_recording[HemisphereOnDLS::bottom].empty();

		LOG4CXX_TRACE(
		    logger, "operator(): Preprocessed local vertices for realtime column "
		                << i << " in " << realtime_preprocess_timer.print() << ".");
	}

	std::vector<std::vector<Timer::Value>> realtime_durations;
	std::vector<std::optional<ExecutionInstanceNode::PeriodicCADCReadoutTimes>>
	    cadc_readout_time_information;
	std::optional<std::vector<Timer::Value>> times_of_first_cadc_sample;
	std::vector<Timer::Value> current_times; // beginning of the realtime section of the snippets
	std::vector<Timer::Value>
	    subsequent_times; // beginning of the realtime section of the subsequent snippets
	for (size_t i = 0; i < realtime_column_count; i++) {
		// build realtime programs
		hate::Timer const realtime_generate_timer;
		auto realtime_column = builders[i].generate(usages_before[i], usages_after[i]);

		// initialize current_times, subsequent_times and realtime_durations
		if (i == 0) {
			current_times =
			    std::vector<Timer::Value>(realtime_column.realtimes.size(), Timer::Value(0));
			subsequent_times =
			    std::vector<Timer::Value>(realtime_column.realtimes.size(), Timer::Value(0));
		} else {
			current_times = subsequent_times;
		}
		// Set subsequent times to the begin of the subsequent realtime snippet and collect the
		// realtime durations for calculating the approximated CADC sample size
		std::vector<Timer::Value> realtime_durations_local_column;
		for (size_t j = 0; j < realtime_column.realtimes.size(); j++) {
			realtime_durations_local_column.push_back(
			    realtime_column.realtimes[j].realtime_duration);
			subsequent_times[j] = current_times[j] + realtime_column.realtimes[j].realtime_duration;
		}
		realtime_durations.push_back(realtime_durations_local_column);
		if (periodic_cadc_recording[i] || periodic_cadc_dram_recording[i]) {
			if (!times_of_first_cadc_sample) {
				std::vector<Timer::Value> determined_times_of_first_cadc_sample;
				for (size_t j = 0; j < realtime_column.realtimes.size(); j++) {
					determined_times_of_first_cadc_sample.push_back(current_times[j]);
				}
				times_of_first_cadc_sample = std::move(determined_times_of_first_cadc_sample);
			}
			cadc_readout_time_information.push_back(ExecutionInstanceNode::PeriodicCADCReadoutTimes{
			    .time_zero = times_of_first_cadc_sample.value(),
			    .interval_begin = current_times,
			    .interval_end = subsequent_times});
		} else {
			cadc_readout_time_information.push_back(std::nullopt);
		}

		realtime_columns.push_back(std::move(realtime_column));
		LOG4CXX_TRACE(
		    logger, "operator(): Generated playback program builders and processed CADC readout "
		            "time information for realtime column "
		                << i << " in " << realtime_generate_timer.print() << ".");
	}

	// Construct cadc_finalize_builders, which each are executed at the end of a batch entry
	std::vector<PlaybackProgramBuilder> cadc_finalize_builders(
	    realtime_columns[0].realtimes.size());
	std::vector<typed_array<std::optional<ContainerTicket>, PPUOnDLS>> cadc_readout_tickets(
	    realtime_columns[0].realtimes.size(),
	    typed_array<std::optional<ContainerTicket>, PPUOnDLS>{});
	auto const has_periodic_cadc_recording =
	    std::find(periodic_cadc_recording.begin(), periodic_cadc_recording.end(), true) !=
	    periodic_cadc_recording.end();
	auto const has_periodic_cadc_dram_recording =
	    std::find(periodic_cadc_dram_recording.begin(), periodic_cadc_dram_recording.end(), true) !=
	    periodic_cadc_dram_recording.end();
	assert(!has_periodic_cadc_recording || !has_periodic_cadc_dram_recording);
	if (has_periodic_cadc_recording || has_periodic_cadc_dram_recording) {
		// generate tickets for extmem readout of periodic cadc recording data
		for (size_t i = 0; i < realtime_columns[0].realtimes.size(); i++) {
			auto [cadc_finalize_builder, local_cadc_readout_tickets] =
			    generate(generator::PPUPeriodicCADCRead(
			        {uses_top_cadc, uses_bot_cadc}, *m_ppu_program.symbols));
			cadc_finalize_builders.at(i) = std::move(cadc_finalize_builder);
			cadc_readout_tickets[i] = std::move(local_cadc_readout_tickets.tickets);
		}
	}

	Program program;
	PostProcessor post_processor;

	program.realtime_columns = std::move(realtime_columns);
	program.cadc_finalize_builders = std::move(cadc_finalize_builders);
	program.uses_madc = uses_madc;

	post_processor.cadc_readout_tickets = std::move(cadc_readout_tickets);
	post_processor.cadc_readout_time_information = std::move(cadc_readout_time_information);
	post_processor.snippet_executors = std::move(builders);

	return {std::move(program), std::move(post_processor)};
}

} // namespace grenade::vx::execution::detail
