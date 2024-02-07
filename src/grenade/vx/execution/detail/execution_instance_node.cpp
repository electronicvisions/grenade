#include "grenade/vx/execution/detail/execution_instance_node.h"

#include "fisch/vx/omnibus.h"
#include "fisch/vx/playback_program_builder.h"
#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/backend/run.h"
#include "grenade/vx/execution/detail/execution_instance_config_visitor.h"
#include "grenade/vx/ppu/detail/status.h"
#include "grenade/vx/ppu/detail/stopped.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/omnibus_constants.h"
#include "haldls/vx/v3/timer.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include "stadls/visitors.h"
#include "stadls/vx/constants.h"
#include "stadls/vx/v3/container_ticket.h"

#include <utility>
#include <log4cxx/logger.h>

namespace grenade::vx::execution::detail {

namespace {

/**
 * Helper to only encode the constant addresses of the lola Chip object once.
 */
static const std::vector<halco::hicann_dls::vx::OmnibusAddress> chip_addresses = []() {
	using namespace halco::hicann_dls::vx::v3;
	typedef std::vector<OmnibusAddress> addresses_type;
	addresses_type storage;
	lola::vx::v3::Chip config;
	haldls::vx::visit_preorder(
	    config, ChipOnDLS(), stadls::WriteAddressVisitor<addresses_type>{storage});
	return storage;
}();

/**
 * Helper to check whether a collection of addresses contains CapMem value configuration.
 * @param addresses Addresses to check
 * @return Boolean value.
 */
bool contains_capmem(std::vector<halco::hicann_dls::vx::OmnibusAddress> const& addresses)
{
	// iterate over all addresses and check whether the base matches one of the CapMem base
	// addresses.
	for (auto const& address : addresses) {
		// select only the upper 16 bit and compare to the CapMem base address.
		auto const base = (address.value() & 0xffff0000);
		// north-west and south-west base addresses suffice because east base addresses only have
		// another bit set in the lower 16 bit.
		if ((base == haldls::vx::v3::capmem_nw_sram_base_address) ||
		    (base == haldls::vx::v3::capmem_sw_sram_base_address)) {
			return true;
		}
	}
	return false;
}

} // namespace

ExecutionInstanceNode::ExecutionInstanceNode(
    std::vector<signal_flow::OutputData>& data_maps,
    std::vector<std::reference_wrapper<signal_flow::InputData const>> const& input_data_maps,
    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
    common::ExecutionInstanceID const& execution_instance,
    halco::hicann_dls::vx::v3::DLSGlobal const& dls_global,
    std::vector<std::reference_wrapper<lola::vx::v3::Chip const>> const& configs,
    backend::Connection& connection,
    ConnectionStateStorage& connection_state_storage,
    signal_flow::ExecutionInstancePlaybackHooks& playback_hooks) :
    data_maps(data_maps),
    input_data_maps(input_data_maps),
    graphs(graphs),
    execution_instance(execution_instance),
    dls_global(dls_global),
    configs(configs),
    connection(connection),
    connection_state_storage(connection_state_storage),
    playback_hooks(playback_hooks),
    logger(log4cxx::Logger::getLogger("grenade.ExecutionInstanceNode"))
{}

void ExecutionInstanceNode::operator()(tbb::flow::continue_msg)
{
	using namespace stadls::vx::v3;
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;

	hate::Timer const build_timer;

	bool const has_hook_around_realtime =
	    !playback_hooks.pre_realtime.empty() || !playback_hooks.inside_realtime_begin.empty() ||
	    !playback_hooks.inside_realtime_end.empty() || !playback_hooks.post_realtime.empty();

	std::optional<lola::vx::v3::PPUElfFile::symbols_type> ppu_symbols;
	if (configs.size() != 1) {
		// check, that no playback hooks are used when having multiple realtime columns
		if (!playback_hooks.pre_static_config.empty() || !playback_hooks.pre_realtime.empty() ||
		    !playback_hooks.inside_realtime_begin.empty() ||
		    !playback_hooks.inside_realtime.empty() ||
		    !playback_hooks.inside_realtime_end.empty() || !playback_hooks.post_realtime.empty() ||
		    !playback_hooks.write_ppu_symbols.empty() || !playback_hooks.read_ppu_symbols.empty()) {
			throw std::logic_error("playback hooks cannot be used in multi-config-experiments");
		}
	}

	// vector for storing all execution_instance_builders
	std::vector<ExecutionInstanceBuilder> builders;
	std::vector<lola::vx::v3::Chip> configs_visited;
	PlaybackProgramBuilder arm_madc;
	std::vector<ExecutionInstanceBuilder::Ret> realtime_columns;

	// vectors for storing the information on what chip components are used in the realtime snippets before and after the current one (the according index)
	// Example: usages_before[0] holds the usages needed before the first realtime snippet etc...
	std::vector<ExecutionInstanceBuilder::Usages> usages_before(configs.size());
	std::vector<ExecutionInstanceBuilder::Usages> usages_after(configs.size());
	usages_before[0] = ExecutionInstanceBuilder::Usages{.madc_recording = false, .event_recording = false};
	usages_after[usages_after.size()-1] = ExecutionInstanceBuilder::Usages{.madc_recording = false, .event_recording = false};

	for (size_t i = 0; i < configs.size(); i++) {
		hate::Timer const initial_config_timer;
		lola::vx::v3::Chip config = configs[i];
		ppu_symbols =
		    std::get<1>(ExecutionInstanceConfigVisitor(graphs[i], execution_instance, config)());
		// check, that no realtime_snippet uses the ppu at all when having multiple realtime columns
		if (configs.size() > 1) {
			if (ppu_symbols) {
				throw std::runtime_error("PPU cannot be used in multi-config-experiments");
			}
		}
		// inject playback-hook PPU symbols to be overwritten
		for (auto const& [name, memory_config] : playback_hooks.write_ppu_symbols) {
			if (!ppu_symbols) {
				throw std::runtime_error("Provided PPU symbols but no PPU program is present.");
			}
			if (!ppu_symbols->contains(name)) {
				throw std::runtime_error(
				    "Provided unknown symbol name via ExecutionInstancePlaybackHooks.");
			}
			std::visit(
			    hate::overloaded{
			        [&](PPUMemoryBlockOnPPU const& coordinate) {
				        for (auto const& [hemisphere, memory] :
				             std::get<std::map<HemisphereOnDLS, haldls::vx::v3::PPUMemoryBlock>>(
				                 memory_config)) {
					        config.ppu_memory[hemisphere.toPPUMemoryOnDLS()].set_block(
					            coordinate, memory);
				        }
			        },
			        [&](ExternalPPUMemoryBlockOnFPGA const& coordinate) {
				        config.external_ppu_memory.set_subblock(
				            coordinate.toMin().value(),
				            std::get<lola::vx::v3::ExternalPPUMemoryBlock>(memory_config));
			        }},
			    ppu_symbols->at(name).coordinate);
		}
		LOG4CXX_TRACE(
		    logger, "operator(): Constructed initial configuration in "
		                << initial_config_timer.print() << ".");

		ExecutionInstanceBuilder builder(
		    graphs[i], execution_instance, input_data_maps[i], data_maps[i], ppu_symbols,
		    playback_hooks);
		builders.push_back(std::move(builder));

		hate::Timer const realtime_preprocess_timer;
		ExecutionInstanceBuilder::Usages usages = builders[i].pre_process();
		if(i < usages_before.size() - 1){
			usages_before[i + 1] = usages;
		}
		if(i > 0){
			usages_after[i - 1] = usages;
		}

		configs_visited.push_back(std::move(config));

		LOG4CXX_TRACE(
		    logger, "operator(): Preprocessed local vertices for realtime section in "
		                << realtime_preprocess_timer.print() << ".");
	}

	for(size_t i = 0; i < configs.size(); i++){
		// build realtime programs
		hate::Timer const realtime_generate_timer;
		auto realtime_column = builders[i].generate(usages_before[i], usages_after[i]);
		realtime_columns.push_back(std::move(realtime_column));
		// check, if madc is used at all in the entire program and catch arm_madc, if so
		if (!realtime_columns[i].arm_madc.empty()) {
			arm_madc = std::move(realtime_columns[i].arm_madc);
		}
		LOG4CXX_TRACE(
		    logger, "operator(): Generated playback programs for realtime section in "
		                << realtime_generate_timer.print() << ".");
	}

	std::vector<PlaybackProgram> programs;
	PlaybackProgramBuilder final_builder;
	for (size_t i = 0; i < realtime_columns[0].realtimes.size(); i++) {
		AbsoluteTimePlaybackProgramBuilder program_builder;
		haldls::vx::v3::Timer::Value config_time =
		    realtime_columns[0].realtimes[i].pre_realtime_duration;
		for (size_t j = 0; j < realtime_columns.size(); j++) {
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
		if (realtime_columns.size() > 1) {
			assemble_builder.merge_back(arm_madc);
			assemble_builder.merge_back(std::move(program_builder.done()));
			assemble_builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
		} else {
			// for the first batch entry, append start_ppu, arm_madc and pre_realtime hook
			if (i == 0) {
				assemble_builder.merge_back(arm_madc);
				assemble_builder.merge_back(playback_hooks.pre_realtime);
			}
			// append inside_realtime_begin hook
			if (i < realtime_columns[0].realtimes.size() - 1) {
				assemble_builder.copy_back(playback_hooks.inside_realtime_begin);
			} else {
				assemble_builder.merge_back(playback_hooks.inside_realtime_begin);
			}
			// append realtime section
			assemble_builder.merge_back(std::move(program_builder.done()));
			// append inside_realtime_end hook
			if (i < realtime_columns[0].realtimes.size() - 1) {
				assemble_builder.copy_back(playback_hooks.inside_realtime_end);
			} else {
				assemble_builder.merge_back(playback_hooks.inside_realtime_end);
			}
			// append ppu_finish_builder
			assemble_builder.merge_back(realtime_columns[0].realtimes[i].ppu_finish_builder);
			// wait for response data
			assemble_builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
			// for the last batch entry, append post_realtime hook and stop_ppu
			if (i == realtime_columns[0].realtimes.size() - 1) {
				assemble_builder.merge_back(playback_hooks.post_realtime);
				assemble_builder.merge_back(realtime_columns[0].stop_ppu);
			}
		}
		if (input_data_maps[input_data_maps.size() - 1].get().inter_batch_entry_wait.contains(
		        execution_instance)) {
			assemble_builder.block_until(
			    TimerOnDLS(), config_time + input_data_maps[input_data_maps.size() - 1]
			                                    .get()
			                                    .inter_batch_entry_wait.at(execution_instance)
			                                    .toTimerOnFPGAValue());
		}
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
	    .realtime = std::move(programs),
	    .has_hook_around_realtime = has_hook_around_realtime,
	    .has_plasticity = ppu_symbols.has_value()};

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
		using namespace haldls::vx::v3;
		// stop PPUs
		auto const ppu_status_coord =
		    std::get<PPUMemoryBlockOnPPU>(ppu_symbols->at("status").coordinate).toMin();
		auto const ppu_stopped_coord =
		    std::get<PPUMemoryBlockOnPPU>(ppu_symbols->at("stopped").coordinate).toMin();
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			PPUMemoryWord config(
			    PPUMemoryWord::Value(static_cast<uint32_t>(ppu::detail::Status::stop)));
			schedule_out_replacement_builder.write(
			    PPUMemoryWordOnDLS(ppu_status_coord, ppu), config);
		}
		// poll for completion by waiting until PPU is stopped
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			PollingOmnibusBlockConfig config;
			config.set_address(PPUMemoryWord::addresses<PollingOmnibusBlockConfig::Address>(
			                       PPUMemoryWordOnDLS(ppu_stopped_coord, ppu))
			                       .at(0));
			config.set_target(
			    PollingOmnibusBlockConfig::Value(static_cast<uint32_t>(ppu::detail::Stopped::yes)));
			config.set_mask(
			    PollingOmnibusBlockConfig::Value(PollingOmnibusBlockConfig::Value::max));
			schedule_out_replacement_builder.write(PollingOmnibusBlockConfigOnFPGA(), config);
			schedule_out_replacement_builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
			schedule_out_replacement_builder.block_until(
			    PollingOmnibusBlockOnFPGA(), PollingOmnibusBlock());
		}
		// disable inhibit reset
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			PPUControlRegister ctrl;
			ctrl.set_inhibit_reset(false);
			schedule_out_replacement_builder.write(ppu.toPPUControlRegisterOnDLS(), ctrl);
		}
		// only PPU memory is volatile between batch entries and therefore read
		for (auto const ppu : iter_all<PPUMemoryOnDLS>()) {
			schedule_out_replacement_builder.read(ppu);
		}
		// if plasticity is present, synaptic weights are volatile between batch entries, read
		if (program.has_plasticity) {
			for (auto const coord : iter_all<SynapseWeightMatrixOnDLS>()) {
				schedule_out_replacement_builder.read(coord);
			}
		}
		schedule_out_replacement_builder.block_until(
		    BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
	}
	auto schedule_out_replacement_program = schedule_out_replacement_builder.done();
	LOG4CXX_TRACE(
	    logger, "operator(): Generated playback program for schedule out replacement in "
	                << schedule_out_replacement_timer.print() << ".");

	hate::Timer const read_timer;
	PlaybackProgramBuilder read_builder;
	halco::common::typed_array<std::optional<ContainerTicket>, PPUMemoryOnDLS>
	    read_ticket_ppu_memory;
	halco::common::typed_array<std::optional<ContainerTicket>, SynapseWeightMatrixOnDLS>
	    read_ticket_synaptic_weights;
	if (connection_state_storage.enable_differential_config && ppu_symbols) {
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			read_ticket_ppu_memory[ppu.toPPUMemoryOnDLS()].emplace(
			    read_builder.read(ppu.toPPUMemoryOnDLS()));
		}
		for (auto const coord : iter_all<SynapseWeightMatrixOnDLS>()) {
			read_ticket_synaptic_weights[coord].emplace(read_builder.read(coord));
		}
		read_builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
	}
	auto read_program = read_builder.done();
	LOG4CXX_TRACE(
	    logger, "operator(): Generated playback program for read-back of PPU alterations in "
	                << read_timer.print() << ".");

	// wait for CapMem to settle
	auto const capmem_settling_wait_program_generator = []() {
		PlaybackProgramBuilder capmem_settling_wait_builder;
		capmem_settling_wait_builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
		capmem_settling_wait_builder.write(TimerOnDLS(), haldls::vx::v3::Timer());
		haldls::vx::v3::Timer::Value const capmem_settling_time(
		    100000 * haldls::vx::v3::Timer::Value::fpga_clock_cycles_per_us);
		capmem_settling_wait_builder.block_until(TimerOnDLS(), capmem_settling_time);
		return capmem_settling_wait_builder.done();
	};

	std::unique_lock<std::mutex> connection_lock(connection_state_storage.mutex, std::defer_lock);
	// exclusive access to connection_state_storage and connection required from here in
	// differential mode
	if (connection_state_storage.enable_differential_config) {
		connection_lock.lock();
	}

	hate::Timer const initial_config_program_timer;
	PlaybackProgramBuilder base_builder;
	PlaybackProgramBuilder differential_builder;
	bool enforce_base = true;
	bool nothing_changed = false;
	bool has_capmem_changes = false;
	if (!connection_state_storage.enable_differential_config) {
		// generate static configuration
		base_builder.write(ChipOnDLS(), configs_visited[0]);
		has_capmem_changes = true;
	} else if (connection_state_storage.current_config_words.empty() /* first invocation */) {
		typedef std::vector<fisch::vx::word_access_type::Omnibus> words_type;
		haldls::vx::visit_preorder(
		    configs_visited[0], ChipOnDLS(),
		    stadls::EncodeVisitor<words_type>{connection_state_storage.current_config_words});
		fisch::vx::PlaybackProgramBuilder fisch_base_builder;
		std::vector<fisch::vx::Omnibus> base_data;
		for (size_t i = 0; i < connection_state_storage.current_config_words.size(); ++i) {
			base_data.push_back(
			    fisch::vx::Omnibus(connection_state_storage.current_config_words.at(i)));
		}
		fisch_base_builder.write(chip_addresses, base_data);
		base_builder.merge_back(fisch_base_builder);
		has_capmem_changes = true;
	} else {
		enforce_base = false;

		if (configs_visited[0] != connection_state_storage.current_config) {
			typedef std::vector<OmnibusAddress> addresses_type;
			typedef std::vector<fisch::vx::word_access_type::Omnibus> words_type;

			auto const& write_addresses = chip_addresses;

			ChipOnDLS coord;
			words_type data_initial;
			haldls::vx::visit_preorder(
			    configs_visited[0], coord, stadls::EncodeVisitor<words_type>{data_initial});
			words_type& data_current = connection_state_storage.current_config_words;
			assert(data_current.size() == chip_addresses.size());

			fisch::vx::PlaybackProgramBuilder fisch_base_builder;
			fisch::vx::PlaybackProgramBuilder fisch_differential_builder;
			std::vector<fisch::vx::Omnibus> base_data;
			std::vector<fisch::vx::Omnibus> differential_data;
			addresses_type base_addresses;
			addresses_type differential_addresses;
			for (size_t i = 0; i < write_addresses.size(); ++i) {
				if (data_initial.at(i) != data_current.at(i)) {
					differential_addresses.push_back(write_addresses.at(i));
					differential_data.push_back(fisch::vx::Omnibus(data_initial.at(i)));
				} else {
					base_addresses.push_back(write_addresses.at(i));
					base_data.push_back(fisch::vx::Omnibus(data_initial.at(i)));
				}
			}
			data_current = data_initial;
			fisch_differential_builder.write(differential_addresses, differential_data);
			fisch_base_builder.write(base_addresses, base_data);
			base_builder.merge_back(fisch_base_builder);
			differential_builder.merge_back(fisch_differential_builder);
			has_capmem_changes = contains_capmem(differential_addresses);
		} else {
			nothing_changed = true;
		}
	}

	auto base_program = base_builder.done();
	auto differential_program = differential_builder.done();
	LOG4CXX_TRACE(
	    logger, "operator(): Generated playback program of initial config after "
	                << initial_config_program_timer.print() << ".");

	LOG4CXX_TRACE(logger, "operator(): Built PlaybackPrograms in " << build_timer.print() << ".");

	auto const trigger_program = realtime_columns[0].start_ppu.done();

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
		    capmem_settling_wait_program_generator(), std::nullopt, has_capmem_changes);

		// Always write (PPU) trigger reinit and enforce when not empty, i.e. when PPUs are used.
		connection_state_storage.reinit_trigger.set(
		    trigger_program, std::nullopt, !trigger_program.empty());

		// Execute realtime sections
		for (auto& p : program.realtime) {
			backend::run(connection, p);
		}

		// If the PPUs (can) alter state, read it back to update current_config accordingly to
		// represent the actual hardware state.
		if (!read_program.empty()) {
			backend::run(connection, read_program);
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
	hate::Timer const update_state_timer;
	if (connection_state_storage.enable_differential_config) {
		connection_state_storage.current_config = configs_visited[configs_visited.size() - 1];
		if (ppu_symbols) {
			for (auto const ppu : iter_all<PPUMemoryOnDLS>()) {
				assert(read_ticket_ppu_memory[ppu]);
				connection_state_storage.current_config.ppu_memory[ppu] =
				    dynamic_cast<haldls::vx::v3::PPUMemory const&>(
				        read_ticket_ppu_memory[ppu]->get());
			}
			for (auto const coord : iter_all<SynapseWeightMatrixOnDLS>()) {
				assert(read_ticket_synaptic_weights[coord]);
				connection_state_storage.current_config.synapse_blocks[coord.toSynapseBlockOnDLS()]
				    .matrix.weights = dynamic_cast<lola::vx::v3::SynapseWeightMatrix const&>(
				                          read_ticket_synaptic_weights[coord]->get())
				                          .values;
			}
			connection_state_storage.current_config_words.clear();
			haldls::vx::visit_preorder(
			    connection_state_storage.current_config, ChipOnDLS(),
			    stadls::EncodeVisitor<std::vector<fisch::vx::word_access_type::Omnibus>>{
			        connection_state_storage.current_config_words});
		}
	}
	LOG4CXX_TRACE(
	    logger, "operator(): Updated connection state in " << update_state_timer.print() << ".");

	// unlock execution section for differential config mode
	if (connection_state_storage.enable_differential_config) {
		if (!program.realtime.empty() || !base_program.empty()) {
			connection_execution_duration_after = connection.get_time_info().execution_duration;
		}
		connection_lock.unlock();
	}

	for (size_t i = 0; i < configs.size(); i++) {
		// extract output data map
		hate::Timer const post_timer;
		auto result_data_map = builders[i].post_process(program.realtime);
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
