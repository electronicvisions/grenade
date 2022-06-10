#include "grenade/vx/execution_instance_node.h"

#include "fisch/vx/omnibus.h"
#include "fisch/vx/playback_program_builder.h"
#include "grenade/vx/backend/connection.h"
#include "grenade/vx/backend/run.h"
#include "grenade/vx/execution_instance_config_visitor.h"
#include "grenade/vx/ppu/status.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/omnibus_constants.h"
#include "haldls/vx/v3/timer.h"
#include "hate/timer.h"
#include "stadls/visitors.h"
#include <log4cxx/logger.h>

namespace grenade::vx {

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
    IODataMap& data_map,
    IODataMap const& input_data_map,
    Graph const& graph,
    coordinate::ExecutionInstance const& execution_instance,
    lola::vx::v3::Chip const& initial_config,
    backend::Connection& connection,
    ConnectionStateStorage& connection_state_storage,
    std::mutex& connection_mutex,
    ExecutionInstancePlaybackHooks& playback_hooks) :
    data_map(data_map),
    input_data_map(input_data_map),
    graph(graph),
    execution_instance(execution_instance),
    initial_config(initial_config),
    connection(connection),
    connection_state_storage(connection_state_storage),
    connection_mutex(connection_mutex),
    playback_hooks(playback_hooks),
    logger(log4cxx::Logger::getLogger("grenade.ExecutionInstanceNode"))
{}

void ExecutionInstanceNode::operator()(tbb::flow::continue_msg)
{
	using namespace stadls::vx::v3;
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;

	std::unique_lock<std::mutex> connection_lock(connection_mutex, std::defer_lock);
	// lock whole function body for differential config mode
	if (connection_state_storage.enable_differential_config) {
		connection_lock.lock();
	}

	hate::Timer const initial_config_timer;
	auto config = initial_config;
	auto const ppu_symbols =
	    std::get<1>(ExecutionInstanceConfigVisitor(graph, execution_instance, config)());
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
	if (connection_state_storage.enable_differential_config &&
	    !playback_hooks.pre_static_config.empty()) {
		throw std::runtime_error(
		    "A pre static config playback hook is not compatible with differential configuration.");
	}
	auto base_builder = std::move(playback_hooks.pre_static_config);
	PlaybackProgramBuilder differential_builder;

	bool enforce_base = true;
	bool nothing_changed = false;
	bool has_capmem_changes = false;
	if (!connection_state_storage.enable_differential_config) {
		// generate static configuration
		base_builder.write(ChipOnDLS(), config);
		has_capmem_changes = true;
	} else if (connection_state_storage.current_config_words.empty() /* first invokation */) {
		typedef std::vector<fisch::vx::word_access_type::Omnibus> words_type;
		haldls::vx::visit_preorder(
		    config, ChipOnDLS(),
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

		if (config != connection_state_storage.current_config) {
			typedef std::vector<OmnibusAddress> addresses_type;
			typedef std::vector<fisch::vx::word_access_type::Omnibus> words_type;

			auto const& write_addresses = chip_addresses;
			LOG4CXX_TRACE(
			    logger, "operator(): after fisch encode addresses " << build_timer.print() << ".");

			ChipOnDLS coord;
			words_type data_initial;
			haldls::vx::visit_preorder(
			    config, coord, stadls::EncodeVisitor<words_type>{data_initial});
			words_type& data_current = connection_state_storage.current_config_words;
			assert(data_current.size() == chip_addresses.size());

			LOG4CXX_TRACE(logger, "operator(): after fisch encode " << build_timer.print() << ".");
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
			LOG4CXX_TRACE(logger, "operator(): after fisch split " << build_timer.print() << ".");
			fisch_differential_builder.write(differential_addresses, differential_data);
			fisch_base_builder.write(base_addresses, base_data);
			LOG4CXX_TRACE(logger, "operator(): after fisch write " << build_timer.print() << ".");
			base_builder.merge_back(fisch_base_builder);
			differential_builder.merge_back(fisch_differential_builder);
			LOG4CXX_TRACE(logger, "operator(): after fisch " << build_timer.print() << ".");
			has_capmem_changes = contains_capmem(differential_addresses);
		} else {
			nothing_changed = true;
		}
	}

	// build realtime programs
	auto program = builder.generate();

	PlaybackProgramBuilder schedule_out_replacement_builder;
	if (ppu_symbols) {
		using namespace haldls::vx::v3;
		// stop PPUs
		auto const ppu_status_coord = ppu_symbols->at("status").coordinate.toMin();
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			PPUMemoryWord config(PPUMemoryWord::Value(static_cast<uint32_t>(ppu::Status::stop)));
			schedule_out_replacement_builder.write(
			    PPUMemoryWordOnDLS(ppu_status_coord, ppu), config);
		}
		// poll for completion by waiting until PPU is asleep
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			PollingOmnibusBlockConfig config;
			config.set_address(
			    PPUStatusRegister::read_addresses<PollingOmnibusBlockConfig::Address>(
			        ppu.toPPUStatusRegisterOnDLS())
			        .at(0));
			config.set_target(PollingOmnibusBlockConfig::Value(static_cast<uint32_t>(true)));
			config.set_mask(PollingOmnibusBlockConfig::Value(0x00000001));
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

	PlaybackProgramBuilder read_builder;
	halco::common::typed_array<
	    std::optional<PlaybackProgram::ContainerTicket<haldls::vx::v3::PPUMemory>>, PPUMemoryOnDLS>
	    read_ticket_ppu_memory;
	halco::common::typed_array<
	    std::optional<PlaybackProgram::ContainerTicket<lola::vx::v3::SynapseWeightMatrix>>,
	    SynapseWeightMatrixOnDLS>
	    read_ticket_synaptic_weights;
	if (connection_state_storage.enable_differential_config) {
		if (ppu_symbols) {
			for (auto const ppu : iter_all<PPUOnDLS>()) {
				read_ticket_ppu_memory[ppu.toPPUMemoryOnDLS()].emplace(
				    read_builder.read(ppu.toPPUMemoryOnDLS()));
			}
			for (auto const coord : iter_all<SynapseWeightMatrixOnDLS>()) {
				read_ticket_synaptic_weights[coord].emplace(read_builder.read(coord));
			}
		}
	}
	LOG4CXX_TRACE(logger, "operator(): all inits but trigger after " << build_timer.print() << ".");

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

	// bring PPUs in running state
	PlaybackProgramBuilder trigger_builder;
	if (ppu_symbols) {
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			haldls::vx::v3::PPUControlRegister ctrl;
			ctrl.set_inhibit_reset(true);
			trigger_builder.write(ppu.toPPUControlRegisterOnDLS(), ctrl);
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
			trigger_builder.write(PollingOmnibusBlockConfigOnFPGA(), config);
			trigger_builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
			trigger_builder.block_until(PollingOmnibusBlockOnFPGA(), PollingOmnibusBlock());
		}
	}
	auto trigger_program = trigger_builder.done();

	auto base_program = base_builder.done();
	auto differential_program = differential_builder.done();
	auto schedule_out_replacement_program = schedule_out_replacement_builder.done();
	LOG4CXX_TRACE(logger, "operator(): all inits after " << build_timer.print() << ".");

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
	if (!program.realtime.empty() || !base_program.empty()) {
		// only lock execution section for non-differential config mode
		if (!connection_state_storage.enable_differential_config) {
			connection_lock.lock();
		}

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
		if (!read_builder.empty()) {
			backend::run(connection, read_builder.done());
		}

		// unlock execution section for non-differential config mode
		if (!connection_state_storage.enable_differential_config) {
			connection_lock.unlock();
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

	// update connection state
	if (connection_state_storage.enable_differential_config) {
		connection_state_storage.current_config = config;
		if (ppu_symbols) {
			for (auto const ppu : iter_all<PPUMemoryOnDLS>()) {
				assert(read_ticket_ppu_memory[ppu]);
				connection_state_storage.current_config.ppu_memory[ppu] =
				    read_ticket_ppu_memory[ppu]->get();
			}
			for (auto const coord : iter_all<SynapseWeightMatrixOnDLS>()) {
				assert(read_ticket_synaptic_weights[coord]);
				connection_state_storage.current_config.synapse_blocks[coord.toSynapseBlockOnDLS()]
				    .matrix.weights = read_ticket_synaptic_weights[coord]->get().values;
			}
			connection_state_storage.current_config_words.clear();
			haldls::vx::visit_preorder(
			    connection_state_storage.current_config, ChipOnDLS(),
			    stadls::EncodeVisitor<std::vector<fisch::vx::word_access_type::Omnibus>>{
			        connection_state_storage.current_config_words});
		}
	}
}

} // namespace grenade::vx
