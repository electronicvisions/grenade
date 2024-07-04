#include "grenade/vx/execution/detail/execution_instance_node.h"

#include "fisch/vx/omnibus.h"
#include "fisch/vx/playback_program_builder.h"
#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/backend/run.h"
#include "grenade/vx/execution/detail/execution_instance_builder.h"
#include "grenade/vx/execution/detail/execution_instance_config_visitor.h"
#include "grenade/vx/execution/detail/generator/capmem.h"
#include "grenade/vx/execution/detail/generator/get_state.h"
#include "grenade/vx/execution/detail/generator/madc.h"
#include "grenade/vx/execution/detail/generator/ppu.h"
#include "grenade/vx/execution/detail/ppu_program_generator.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/detail/status.h"
#include "grenade/vx/ppu/detail/stopped.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/omnibus_constants.h"
#include "haldls/vx/v3/timer.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include "stadls/visitors.h"
#include "stadls/vx/constants.h"
#include "stadls/vx/v3/container_ticket.h"

#include <algorithm>
#include <utility>
#include <log4cxx/logger.h>

namespace grenade::vx::execution::detail {

ExecutionInstanceNode::ExecutionInstanceNode(
    std::vector<signal_flow::OutputData>& data_maps,
    std::vector<std::reference_wrapper<signal_flow::InputData const>> const& input_data_maps,
    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
    common::ExecutionInstanceID const& execution_instance,
    halco::hicann_dls::vx::v3::DLSGlobal const& dls_global,
    std::vector<std::reference_wrapper<lola::vx::v3::Chip const>> const& configs,
    backend::Connection& connection,
    ConnectionStateStorage& connection_state_storage,
    signal_flow::ExecutionInstanceHooks& hooks) :
    data_maps(data_maps),
    input_data_maps(input_data_maps),
    graphs(graphs),
    execution_instance(execution_instance),
    dls_global(dls_global),
    configs(configs),
    connection(connection),
    connection_state_storage(connection_state_storage),
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

	hate::Timer const build_timer;

	assert(
	    (std::set<size_t>{data_maps.size(), input_data_maps.size(), graphs.size(), configs.size()}
	         .size() == 1));
	size_t const realtime_column_count = data_maps.size();

	bool const has_hook_around_realtime =
	    !hooks.pre_realtime.empty() || !hooks.inside_realtime_begin.empty() ||
	    !hooks.inside_realtime_end.empty() || !hooks.post_realtime.empty();

	if (realtime_column_count != 1) {
		// check, that no playback hooks are used when having multiple realtime columns
		if (!hooks.pre_static_config.empty() || !hooks.pre_realtime.empty() ||
		    !hooks.inside_realtime_begin.empty() || !hooks.inside_realtime.empty() ||
		    !hooks.inside_realtime_end.empty() || !hooks.post_realtime.empty() ||
		    !hooks.write_ppu_symbols.empty() || !hooks.read_ppu_symbols.empty()) {
			throw std::logic_error("playback hooks cannot be used in multi-config-experiments");
		}
	}

	// vector for storing all execution_instance_builders
	std::vector<ExecutionInstanceBuilder> builders;
	std::vector<Chip> configs_visited;
	std::vector<ExecutionInstanceBuilder::Ret> realtime_columns;

	// vectors for storing the information on what chip components are used in the realtime snippets before and after the current one (the according index)
	// Example: usages_before[0] holds the usages needed before the first realtime snippet etc...
	std::vector<ExecutionInstanceBuilder::Usages> usages_before(configs.size());
	std::vector<ExecutionInstanceBuilder::Usages> usages_after(configs.size());
	usages_before[0] = ExecutionInstanceBuilder::Usages{
	    .madc_recording = false,
	    .event_recording = false,
	    .cadc_recording = typed_array<
	        std::set<signal_flow::vertex::CADCMembraneReadoutView::Mode>, HemisphereOnDLS>{{}}};
	usages_after[usages_after.size() - 1] = ExecutionInstanceBuilder::Usages{
	    .madc_recording = false,
	    .event_recording = false,
	    .cadc_recording = typed_array<
	        std::set<signal_flow::vertex::CADCMembraneReadoutView::Mode>, HemisphereOnDLS>{{}}};

	ExecutionInstanceConfigVisitor::PpuUsage overall_ppu_usage;
	std::vector<typed_array<bool, NeuronResetOnDLS>> enabled_neuron_resets;
	hate::Timer const configs_timer;
	for (size_t i = 0; i < realtime_column_count; i++) {
		Chip config = configs[i];
		std::tuple<ExecutionInstanceConfigVisitor::PpuUsage, typed_array<bool, NeuronResetOnDLS>>
		    ppu_information =
		        ExecutionInstanceConfigVisitor(graphs[i], execution_instance, config, i)();
		configs_visited.push_back(std::move(config));
		overall_ppu_usage += std::move(std::get<0>(ppu_information));
		enabled_neuron_resets.push_back(std::get<1>(ppu_information));
	}

	std::optional<PPUElfFile::symbols_type> ppu_symbols;
	PPUMemoryBlockOnPPU ppu_status_coord;
	if (overall_ppu_usage.has_cadc_readout || !overall_ppu_usage.plasticity_rules.empty()) {
		hate::Timer ppu_timer;
		PPUElfFile::Memory ppu_program;
		PPUMemoryBlockOnPPU ppu_neuron_reset_mask_coord;
		PPUMemoryWordOnPPU ppu_location_coord;
		{
			PPUProgramGenerator ppu_program_generator;
			for (auto const& [descriptor, rule, synapses, neurons, realtime_column_index] :
			     overall_ppu_usage.plasticity_rules) {
				ppu_program_generator.add(
				    descriptor, rule, synapses, neurons, realtime_column_index);
			}
			ppu_program_generator.has_periodic_cadc_readout =
			    overall_ppu_usage.has_periodic_cadc_readout;
			CachingCompiler compiler;
			auto const program = compiler.compile(ppu_program_generator.done());
			ppu_program = program.memory;
			ppu_symbols = program.symbols;
			ppu_neuron_reset_mask_coord =
			    std::get<PPUMemoryBlockOnPPU>(ppu_symbols->at("neuron_reset_mask").coordinate);
			ppu_location_coord =
			    std::get<PPUMemoryBlockOnPPU>(ppu_symbols->at("ppu").coordinate).toMin();
			ppu_status_coord = std::get<PPUMemoryBlockOnPPU>(ppu_symbols->at("status").coordinate);
		}
		LOG4CXX_TRACE(logger, "Generated PPU program in " << ppu_timer.print() << ".");

		for (size_t i = 0; i < realtime_column_count; i++) {
			for (auto const ppu : iter_all<PPUOnDLS>()) {
				// zero-initialize all symbols (esp. bss)
				assert(ppu_symbols);
				for (auto const& [_, symbol] : *ppu_symbols) {
					std::visit(
					    hate::overloaded{
					        [&](PPUMemoryBlockOnPPU const& coordinate) {
						        PPUMemoryBlock block(coordinate.toPPUMemoryBlockSize());
						        configs_visited[i].ppu_memory[ppu.toPPUMemoryOnDLS()].set_block(
						            coordinate, block);
					        },
					        [&](ExternalPPUMemoryBlockOnFPGA const& coordinate) {
						        ExternalPPUMemoryBlock block(
						            coordinate.toExternalPPUMemoryBlockSize());
						        configs_visited[i].external_ppu_memory.set_subblock(
						            coordinate.toMin().value(), block);
					        }},
					    symbol.coordinate);
				}

				// set internal PPU program
				configs_visited[i].ppu_memory[ppu.toPPUMemoryOnDLS()].set_block(
				    PPUMemoryBlockOnPPU(
				        PPUMemoryWordOnPPU(),
				        PPUMemoryWordOnPPU(ppu_program.internal.get_words().size() - 1)),
				    ppu_program.internal);

				// set external PPU program
				if (ppu_program.external) {
					configs_visited[i].external_ppu_memory.set_subblock(
					    0, ppu_program.external.value());
				}

				// set neuron reset mask
				halco::common::typed_array<int8_t, NeuronColumnOnDLS> values;
				for (auto const col : iter_all<NeuronColumnOnDLS>()) {
					values[col] =
					    enabled_neuron_resets[i][AtomicNeuronOnDLS(col, ppu.toNeuronRowOnDLS())
					                                 .toNeuronResetOnDLS()];
				}
				auto const neuron_reset_mask = to_vector_unit_row(values);
				configs_visited[i].ppu_memory[ppu.toPPUMemoryOnDLS()].set_block(
				    ppu_neuron_reset_mask_coord, neuron_reset_mask);
				configs_visited[i].ppu_memory[ppu.toPPUMemoryOnDLS()].set_word(
				    ppu_location_coord, PPUMemoryWord::Value(ppu.value()));
			}

			// inject playback-hook PPU symbols to be overwritten
			for (auto const& [name, memory_config] : hooks.write_ppu_symbols) {
				if (!ppu_symbols) {
					throw std::runtime_error("Provided PPU symbols but no PPU program is present.");
				}
				if (!ppu_symbols->contains(name)) {
					throw std::runtime_error(
					    "Provided unknown symbol name via ExecutionInstanceHooks.");
				}
				std::visit(
				    hate::overloaded{
				        [&](PPUMemoryBlockOnPPU const& coordinate) {
					        for (auto const& [hemisphere, memory] : std::get<
					                 std::map<HemisphereOnDLS, haldls::vx::v3::PPUMemoryBlock>>(
					                 memory_config)) {
						        configs_visited[i]
						            .ppu_memory[hemisphere.toPPUMemoryOnDLS()]
						            .set_block(coordinate, memory);
					        }
				        },
				        [&](ExternalPPUMemoryBlockOnFPGA const& coordinate) {
					        configs_visited[i].external_ppu_memory.set_subblock(
					            coordinate.toMin().value(),
					            std::get<lola::vx::v3::ExternalPPUMemoryBlock>(memory_config));
				        }},
				    ppu_symbols->at(name).coordinate);
			}
		}
	}

	LOG4CXX_TRACE(
	    logger, "operator(): Constructed configuration(s) in " << configs_timer.print() << ".");

	std::vector<bool> periodic_cadc_recording;
	bool uses_top_cadc = false;
	bool uses_bot_cadc = false;
	bool uses_madc = false;
	for (size_t i = 0; i < realtime_column_count; i++) {
		ExecutionInstanceBuilder builder(
		    graphs[i], execution_instance, input_data_maps[i], data_maps[i], ppu_symbols, hooks, i);
		builders.push_back(std::move(builder));

		hate::Timer const realtime_preprocess_timer;
		ExecutionInstanceBuilder::Usages usages = builders[i].pre_process();
		if (usages.madc_recording) {
			uses_madc = true;
		}
		if(i < usages_before.size() - 1){
			usages_before[i + 1] = usages;
		}
		if(i > 0){
			usages_after[i - 1] = usages;
		}
		periodic_cadc_recording.push_back(std::any_of(
		    usages.cadc_recording.begin(), usages.cadc_recording.end(),
		    [](std::set<signal_flow::vertex::CADCMembraneReadoutView::Mode> const& mode) {
			    return mode.contains(signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic);
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
		if (periodic_cadc_recording[i]) {
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
	std::vector<PlaybackProgramBuilder> cadc_finalize_builders;
	std::vector<typed_array<std::optional<ContainerTicket>, PPUOnDLS>> cadc_readout_tickets(
	    realtime_columns[0].realtimes.size(),
	    typed_array<std::optional<ContainerTicket>, PPUOnDLS>{});
	if (std::find(periodic_cadc_recording.begin(), periodic_cadc_recording.end(), true) !=
	    periodic_cadc_recording.end()) {
		PlaybackProgramBuilder cadc_finalize_builder_base;

		InstructionTimeoutConfig instruction_timeout;
		instruction_timeout.set_value(InstructionTimeoutConfig::Value(
		    100000 * InstructionTimeoutConfig::Value::fpga_clock_cycles_per_us));
		cadc_finalize_builder_base.write(
		    halco::hicann_dls::vx::InstructionTimeoutConfigOnFPGA(), instruction_timeout);

		// wait for ppu command idle
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			PPUMemoryWordOnPPU ppu_status_coord;
			ppu_status_coord =
			    std::get<PPUMemoryBlockOnPPU>(ppu_symbols->at("status").coordinate).toMin();
			PollingOmnibusBlockConfig polling_config;
			polling_config.set_address(PPUMemoryWord::addresses<PollingOmnibusBlockConfig::Address>(
			                               PPUMemoryWordOnDLS(ppu_status_coord, ppu))
			                               .at(0));
			polling_config.set_target(
			    PollingOmnibusBlockConfig::Value(static_cast<uint32_t>(ppu::detail::Status::idle)));
			polling_config.set_mask(PollingOmnibusBlockConfig::Value(0xffffffff));
			cadc_finalize_builder_base.write(PollingOmnibusBlockConfigOnFPGA(), polling_config);
			cadc_finalize_builder_base.block_until(BarrierOnFPGA(), Barrier::omnibus);
			cadc_finalize_builder_base.block_until(
			    PollingOmnibusBlockOnFPGA(), PollingOmnibusBlock());

			cadc_finalize_builder_base.write(
			    halco::hicann_dls::vx::InstructionTimeoutConfigOnFPGA(),
			    InstructionTimeoutConfig());
		}

		// generate tickets for extmem readout of periodic cadc recording data
		for (size_t i = 0; i < realtime_columns[0].realtimes.size(); i++) {
			PlaybackProgramBuilder cadc_finalize_builder;
			if (i < realtime_columns[0].realtimes.size() - 1) {
				cadc_finalize_builder.copy_back(cadc_finalize_builder_base);
			} else {
				cadc_finalize_builder.merge_back(cadc_finalize_builder_base);
			}
			// calculate expected time durations where periodic_cadc_recording is enabled
			Timer::Value expected_pcr_time = Timer::Value(0);
			for (size_t j = 0; j < realtime_column_count; j++) {
				if (periodic_cadc_recording[j]) {
					expected_pcr_time += realtime_durations[j][i];
				}
			}
			// calculate aproximate number of expected samples
			size_t estimated_size = 0;
			// lower bound on sample duration -> upper bound on sample rate
			size_t const approx_sample_duration =
			    static_cast<size_t>(1.7 * Timer::Value::fpga_clock_cycles_per_us);
			estimated_size = expected_pcr_time / approx_sample_duration;
			// add a sample as constant margin for error
			estimated_size += 1;
			// cap at maximal possible amount of samples
			if (estimated_size > static_cast<size_t>(100)) {
				LOG4CXX_WARN(
				    logger, "It is estimated, that the CADC will measure "
				                << estimated_size << " samples in batch entry " << i
				                << ", but only the first 100 can be recorded.");
				estimated_size = static_cast<size_t>(100);
			}
			if (uses_top_cadc) {
				cadc_readout_tickets[i][PPUOnDLS::top] =
				    cadc_finalize_builder.read(ExternalPPUMemoryBlockOnFPGA(
				        std::get<ExternalPPUMemoryBlockOnFPGA>(
				            ppu_symbols->at("periodic_cadc_samples_top").coordinate)
				            .toMin(),
				        ExternalPPUMemoryByteOnFPGA(
				            std::get<ExternalPPUMemoryBlockOnFPGA>(
				                ppu_symbols->at("periodic_cadc_samples_top").coordinate)
				                .toMin() +
				            128 + ((256 + 128) * estimated_size) - 1)));
			}
			if (uses_bot_cadc) {
				cadc_readout_tickets[i][PPUOnDLS::bottom] =
				    cadc_finalize_builder.read(ExternalPPUMemoryBlockOnFPGA(
				        std::get<ExternalPPUMemoryBlockOnFPGA>(
				            ppu_symbols->at("periodic_cadc_samples_bot").coordinate)
				            .toMin(),
				        ExternalPPUMemoryByteOnFPGA(
				            std::get<ExternalPPUMemoryBlockOnFPGA>(
				                ppu_symbols->at("periodic_cadc_samples_bot").coordinate)
				                .toMin() +
				            128 + ((256 + 128) * estimated_size) - 1)));
			}
			// Reset the offset, from where the PPU begins to write the recorded CADC data
			for (auto const ppu : iter_all<PPUOnDLS>()) {
				cadc_finalize_builder.write(
				    PPUMemoryWordOnDLS(
				        std::get<PPUMemoryBlockOnPPU>(
				            ppu_symbols->at("periodic_cadc_readout_memory_offset").coordinate)
				            .toMin(),
				        ppu),
				    PPUMemoryWord(PPUMemoryWord::Value(static_cast<uint32_t>(0))));
			}
			cadc_finalize_builders.push_back(std::move(cadc_finalize_builder));
		}
	}

	// Experiment assembly
	std::vector<PlaybackProgram> programs;
	PlaybackProgramBuilder final_builder;
	for (size_t i = 0; i < realtime_columns[0].realtimes.size(); i++) {
		AbsoluteTimePlaybackProgramBuilder program_builder;
		haldls::vx::v3::Timer::Value config_time =
		    realtime_columns[0].realtimes[i].pre_realtime_duration;
		for (size_t j = 0; j < realtime_column_count; j++) {
			if (j > 0) {
				config_time += realtime_columns[j - 1].realtimes[i].realtime_duration;
				program_builder.write(
				    config_time, ChipOnDLS(), configs_visited[j], configs_visited[j - 1]);
			}
			realtime_columns[j].realtimes[i].builder +=
			    (config_time - realtime_columns[j].realtimes[i].pre_realtime_duration);
			program_builder.merge(realtime_columns[j].realtimes[i].builder);
		}

		// reset config to initial config at end of each realtime_row if experiment has multiple
		// configs
		if (realtime_columns.size() > 1) {
			config_time +=
			    realtime_columns[realtime_columns.size() - 1].realtimes[i].realtime_duration;
			if (i < realtime_columns[0].realtimes.size() - 1) {
				program_builder.write(
				    config_time, ChipOnDLS(), configs_visited[0],
				    configs_visited[configs_visited.size() - 1]);
			}
		}

		// assemble playback_program from arm_madc and program_builder and if applicable, start_ppu,
		// stop_ppu and the playback hooks
		PlaybackProgramBuilder assemble_builder;
		if (i == 0 && uses_madc) {
			assemble_builder.merge_back(generate(generator::MADCArm()).builder);
		}
		if (realtime_columns.size() > 1) {
			assemble_builder.merge_back(std::move(program_builder.done()));
		} else {
			// for the first batch entry, append start_ppu, arm_madc and pre_realtime hook
			if (i == 0) {
				assemble_builder.merge_back(hooks.pre_realtime);
			}
			// append inside_realtime_begin hook
			if (i < realtime_columns[0].realtimes.size() - 1) {
				assemble_builder.copy_back(hooks.inside_realtime_begin);
			} else {
				assemble_builder.merge_back(hooks.inside_realtime_begin);
			}
			// append realtime section
			assemble_builder.merge_back(std::move(program_builder.done()));
			// append inside_realtime_end hook
			if (i < realtime_columns[0].realtimes.size() - 1) {
				assemble_builder.copy_back(hooks.inside_realtime_end);
			} else {
				assemble_builder.merge_back(hooks.inside_realtime_end);
			}
		}
		// append ppu_finish_builders
		for (size_t j = 0; j < realtime_column_count; j++) {
			assemble_builder.merge_back(realtime_columns[j].realtimes[i].ppu_finish_builder);
		}
		// wait for response data
		assemble_builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
		if (realtime_columns.size() <= 1) {
			// for the last batch entry, append post_realtime hook
			if (i == realtime_columns[0].realtimes.size() - 1) {
				assemble_builder.merge_back(hooks.post_realtime);
			}
		}
		// append cadc_finazlize_builder for periodic_cadc data readout
		if (std::find(periodic_cadc_recording.begin(), periodic_cadc_recording.end(), true) !=
		    periodic_cadc_recording.end()) {
			assemble_builder.merge_back(cadc_finalize_builders.at(i));
		}
		// for the last batch entry, stop the PPU
		if (i == realtime_columns[0].realtimes.size() - 1 && ppu_symbols) {
			assemble_builder.merge_back(
			    generate(generator::PPUStop(ppu_status_coord.toMin())).builder);
		}
		// Implement inter_batch_entry_wait (ensures minimal waiting time in between batch entries)
		if (input_data_maps[input_data_maps.size() - 1].get().inter_batch_entry_wait.contains(
		        execution_instance)) {
			assemble_builder.block_until(
			    TimerOnDLS(), config_time + input_data_maps[input_data_maps.size() - 1]
			                                    .get()
			                                    .inter_batch_entry_wait.at(execution_instance)
			                                    .toTimerOnFPGAValue());
		}
		// Create playback programs with the maximal amount of batch entries that fit in as a whole
		if ((final_builder.size_to_fpga() + assemble_builder.size_to_fpga()) >
		    stadls::vx::playback_memory_size_to_fpga) {
			programs.push_back(std::move(final_builder.done()));
		}
		final_builder.merge_back(assemble_builder);
	}
	if (!final_builder.empty()) {
		programs.push_back(std::move(final_builder.done()));
	}

	PlaybackPrograms program = PlaybackPrograms{
	    .realtime = std::move(programs), .has_hook_around_realtime = has_hook_around_realtime};

	if (program.realtime.size() > 1 && connection.is_quiggeldy() &&
	    program.has_hook_around_realtime) {
		LOG4CXX_WARN(
		    logger, "operator(): Connection uses quiggeldy and more than one playback programs "
		            "shall be executed back-to-back with a pre or post realtime hook. Their "
		            "contiguity can't be guaranteed.");
	}

	hate::Timer const schedule_out_replacement_timer;
	PlaybackProgramBuilder schedule_out_replacement_builder;
	if (ppu_symbols) {
		schedule_out_replacement_builder.merge_back(
		    generate(generator::PPUStop(ppu_status_coord.toMin())).builder);
		schedule_out_replacement_builder.merge_back(
		    generate(generator::GetState{ppu_symbols.has_value()}).builder);
	}
	auto schedule_out_replacement_program = schedule_out_replacement_builder.done();
	LOG4CXX_TRACE(
	    logger, "operator(): Generated playback program for schedule out replacement in "
	                << schedule_out_replacement_timer.print() << ".");

	hate::Timer const read_timer;
	PlaybackProgram get_state_program;
	std::optional<generator::GetState::Result> get_state_result;
	if (connection_state_storage.enable_differential_config && ppu_symbols) {
		auto get_state = generate(generator::GetState(ppu_symbols.has_value()));
		get_state_program = get_state.builder.done();
		get_state_result = get_state.result;
	}
	LOG4CXX_TRACE(
	    logger, "operator(): Generated playback program for read-back of PPU alterations in "
	                << read_timer.print() << ".");

	std::unique_lock<std::mutex> connection_lock(connection_state_storage.mutex, std::defer_lock);
	// exclusive access to connection_state_storage and connection required from here in
	// differential mode
	if (connection_state_storage.enable_differential_config) {
		connection_lock.lock();
	}

	hate::Timer const initial_config_program_timer;
	bool const is_fresh_connection_state_storage_config =
	    connection_state_storage.config.get_is_fresh();
	connection_state_storage.config.set_chip(configs_visited[0], true);

	bool const enforce_base = !connection_state_storage.config.get_enable_differential_config() ||
	                          is_fresh_connection_state_storage_config;
	bool const has_capmem_changes =
	    !connection_state_storage.config.get_enable_differential_config() ||
	    is_fresh_connection_state_storage_config ||
	    connection_state_storage.config.get_differential_changes_capmem();
	bool const nothing_changed = connection_state_storage.config.get_enable_differential_config() &&
	                             !is_fresh_connection_state_storage_config &&
	                             !connection_state_storage.config.get_has_differential();

	PlaybackProgramBuilder base_builder;
	PlaybackProgramBuilder differential_builder;
	if (!nothing_changed) {
		base_builder.write(
		    ConnectionConfigBaseCoordinate(),
		    ConnectionConfigBase(connection_state_storage.config));
		if (connection_state_storage.config.get_has_differential()) {
			differential_builder.write(
			    ConnectionConfigDifferentialCoordinate(),
			    ConnectionConfigDifferential(connection_state_storage.config));
		}
	}

	auto base_program = base_builder.done();
	auto differential_program = differential_builder.done();
	LOG4CXX_TRACE(
	    logger, "operator(): Generated playback program of initial config after "
	                << initial_config_program_timer.print() << ".");

	LOG4CXX_TRACE(logger, "operator(): Built PlaybackPrograms in " << build_timer.print() << ".");

	auto const trigger_program =
	    ppu_symbols ? generate(generator::PPUStart(ppu_status_coord.toMin())).builder.done()
	                : PlaybackProgram();

	// execute
	hate::Timer const exec_timer;
	std::chrono::nanoseconds connection_execution_duration_before{0};
	std::chrono::nanoseconds connection_execution_duration_after{0};

	if (!program.realtime.empty() || !base_program.empty()) {
		// only lock execution section for non-differential config mode
		if (!connection_state_storage.enable_differential_config) {
			connection_lock.lock();
		}

		// we are locked here in any case
		connection_execution_duration_before = connection.get_time_info().execution_duration;

		// Only if something changed (re-)set base and differential reinit
		if (!nothing_changed) {
			connection_state_storage.reinit_base.set(base_program, std::nullopt, enforce_base);

			// Only enforce when not empty to support non-differential mode.
			// In differential mode it is always enforced on changes.
			connection_state_storage.reinit_differential.set(
			    differential_program, std::nullopt, !differential_program.empty());
		}

		// Never enforce, since the reinit is only filled after a schedule-out operation.
		connection_state_storage.reinit_schedule_out_replacement.set(
		    PlaybackProgram(), schedule_out_replacement_program, false);

		// Always write capmem settling wait reinit, but only enforce it when the wait is
		// immediately required, i.e. after changes to the capmem.
		connection_state_storage.reinit_capmem_settling_wait.set(
		    generate(generator::CapMemSettlingWait()).builder.done(), std::nullopt,
		    has_capmem_changes);

		// Always write (PPU) trigger reinit and enforce when not empty, i.e. when PPUs are used.
		connection_state_storage.reinit_trigger.set(
		    trigger_program, std::nullopt, !trigger_program.empty());

		// Execute realtime sections
		for (auto& p : program.realtime) {
			backend::run(connection, p);
		}

		// If the PPUs (can) alter state, read it back to update current_config accordingly to
		// represent the actual hardware state.
		if (!get_state_program.empty()) {
			backend::run(connection, get_state_program);
		}

		// unlock execution section for non-differential config mode
		if (!connection_state_storage.enable_differential_config) {
			connection_execution_duration_after = connection.get_time_info().execution_duration;
			connection_lock.unlock();
		}
	}
	LOG4CXX_TRACE(
	    logger, "operator(): Executed built PlaybackPrograms in " << exec_timer.print() << ".");

	// update connection state
	if (configs_visited.size() > 1 || ppu_symbols) {
		hate::Timer const update_state_timer;
		auto current_config = configs_visited[configs_visited.size() - 1];
		if (get_state_result) {
			get_state_result->apply(current_config);
		}
		connection_state_storage.config.set_chip(current_config, false);
		LOG4CXX_TRACE(
		    logger,
		    "operator(): Updated connection state in " << update_state_timer.print() << ".");
	}

	// unlock execution section for differential config mode
	if (connection_state_storage.enable_differential_config) {
		if (!program.realtime.empty() || !base_program.empty()) {
			connection_execution_duration_after = connection.get_time_info().execution_duration;
		}
		connection_lock.unlock();
	}

	for (size_t i = 0; i < realtime_column_count; i++) {
		// extract output data map
		hate::Timer const post_timer;
		auto result_data_map = builders[i].post_process(
		    program.realtime, cadc_readout_tickets, cadc_readout_time_information[i]);
		LOG4CXX_TRACE(logger, "operator(): Evaluated in " << post_timer.print() << ".");

		// add execution duration per hardware to result data map
		assert(result_data_map.execution_time_info);
		result_data_map.execution_time_info->execution_duration_per_hardware[dls_global] =
		    connection_execution_duration_after - connection_execution_duration_before;

		// add pre-execution config to result data map
		result_data_map.pre_execution_chips[execution_instance] = configs_visited[i];

		// merge local data map into global data map
		data_maps[i].merge(result_data_map);
	}
}

} // namespace grenade::vx::execution::detail
