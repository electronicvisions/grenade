#include "grenade/vx/execution/detail/execution_instance_chip_ppu_program_compiler.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/backend/playback_program.h"
#include "grenade/vx/execution/detail/execution_instance_chip_snippet_ppu_usage_visitor.h"
#include "grenade/vx/execution/detail/execution_instance_chip_snippet_realtime_executor.h"
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
#include <filesystem>
#include <iterator>
#include <vector>
#include <boost/range/combine.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx::execution::detail {

void ExecutionInstanceChipPPUProgramCompiler::Result::apply(
    backend::PlaybackProgram& playback_program) const
{
	if (!internal.empty() &&
	    playback_program.chips.at(chip_on_connection).chip_configs.size() != internal.size()) {
		throw std::invalid_argument(
		    "Number of realtime snippets in playback program doesn't match PPU program.");
	}

	for (size_t i = 0; i < internal.size(); ++i) {
		// set internal PPU program
		for (auto const ppu : halco::common::iter_all<halco::hicann_dls::vx::v3::PPUOnDLS>()) {
			playback_program.chips.at(chip_on_connection)
			    .chip_configs[i]
			    .ppu_memory[ppu.toPPUMemoryOnDLS()]
			    .set_block(
			        halco::hicann_dls::vx::v3::PPUMemoryBlockOnPPU(
			            halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU(0),
			            halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU(internal[i][ppu].size() - 1)),
			        internal[i][ppu]);
		}

		// set external PPU program
		if (external) {
			playback_program.chips.at(chip_on_connection)
			    .chip_configs[i]
			    .external_ppu_memory.set_subblock(0, external.value());
		}
	}

	// set external dram PPU program
	playback_program.chips.at(chip_on_connection).external_ppu_dram_memory_config = external_dram;

	// set PPU symbols
	playback_program.chips.at(chip_on_connection).ppu_symbols = symbols;
}


ExecutionInstanceChipPPUProgramCompiler::ExecutionInstanceChipPPUProgramCompiler(
    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
    signal_flow::InputData const& input_data,
    signal_flow::ExecutionInstanceHooks const& hooks,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::ExecutionInstanceID const& execution_instance) :
    m_graphs(graphs),
    m_input_data(input_data),
    m_hooks(hooks),
    m_chip_on_connection(chip_on_connection),
    m_execution_instance(execution_instance)
{
}

ExecutionInstanceChipPPUProgramCompiler::Result
ExecutionInstanceChipPPUProgramCompiler::operator()() const
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceChipPPUProgramCompiler");

	Result result;
	result.chip_on_connection = m_chip_on_connection;

	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace lola::vx::v3;
	using namespace haldls::vx::v3;

	assert((std::set<size_t>{m_input_data.snippets.size(), m_graphs.size()}.size() == 1));
	size_t const snippet_count = m_graphs.size();

	ExecutionInstanceChipSnippetPPUUsageVisitor::Result overall_ppu_usage;
	std::vector<ExecutionInstanceChipSnippetPPUUsageVisitor::Result> ppu_usages;
	for (size_t i = 0; i < snippet_count; i++) {
		auto ppu_usage = ExecutionInstanceChipSnippetPPUUsageVisitor(
		    m_graphs[i], m_chip_on_connection, m_execution_instance, i)();
		ppu_usages.push_back(ppu_usage);
		overall_ppu_usage += std::move(ppu_usage);
	}

	std::vector<common::Time> maximal_periodic_cadc_runtimes(m_input_data.batch_size());
	for (size_t i = 0; i < snippet_count; ++i) {
		if (ppu_usages.at(i).has_periodic_cadc_readout ||
		    ppu_usages.at(i).has_periodic_cadc_readout_on_dram) {
			for (size_t b = 0; b < maximal_periodic_cadc_runtimes.size(); ++b) {
				maximal_periodic_cadc_runtimes.at(b) +=
				    m_input_data.snippets.at(i).runtime.at(b).at(m_execution_instance);
			}
		}
	}
	common::Time maximal_periodic_cadc_runtime(0);
	if (!maximal_periodic_cadc_runtimes.empty()) {
		maximal_periodic_cadc_runtime = *std::max_element(
		    maximal_periodic_cadc_runtimes.begin(), maximal_periodic_cadc_runtimes.end());
	}

	result.plasticity_rule_timed_recording_start_periods.resize(snippet_count);
	if (overall_ppu_usage.has_cadc_readout || !overall_ppu_usage.plasticity_rules.empty()) {
		PPUMemoryBlockOnPPU ppu_status_coord;
		PPUMemoryBlockOnPPU ppu_stopped_coord;
		size_t estimated_cadc_recording_size = 0;

		hate::Timer ppu_timer;
		PPUElfFile::Memory ppu_program;
		PPUMemoryBlockOnPPU ppu_neuron_reset_mask_coord;
		PPUMemoryWordOnPPU ppu_location_coord;
		{
			// TODO: rename to PPUProgramSourceCompiler, think about naming of the CachingCompiler
			// below
			PPUProgramGenerator ppu_program_generator;
			std::vector<std::map<signal_flow::vertex::PlasticityRule::ID, size_t>>
			    plasticity_rule_timed_recording_num_periods(snippet_count);
			for (auto const& [descriptor, rule, synapses, neurons, snippet_index] :
			     overall_ppu_usage.plasticity_rules) {
				ppu_program_generator.add(descriptor, rule, synapses, neurons, snippet_index);
				plasticity_rule_timed_recording_num_periods.at(snippet_index)[rule.get_id()] =
				    rule.get_timer().num_periods;
			}
			// sum up number of periods for different plasticity rules in order to get the index
			// offset of the rule in the recording data for each realtime snippet.
			for (size_t i = 0; i < snippet_count; ++i) {
				for (size_t j = 0; j < i; ++j) {
					for (auto const& [id, num_periods] :
					     plasticity_rule_timed_recording_num_periods.at(j)) {
						result.plasticity_rule_timed_recording_start_periods.at(i)[id] +=
						    num_periods;
					}
				}
			}
			ppu_program_generator.has_periodic_cadc_readout =
			    overall_ppu_usage.has_periodic_cadc_readout;
			ppu_program_generator.has_periodic_cadc_readout_on_dram =
			    overall_ppu_usage.has_periodic_cadc_readout_on_dram;
			// calculate aproximate number of expected samples
			// lower bound on sample duration -> upper bound on sample rate
			size_t const approx_sample_duration =
			    static_cast<size_t>(1.7 * Timer::Value::fpga_clock_cycles_per_us);
			estimated_cadc_recording_size =
			    (maximal_periodic_cadc_runtime +
			     (ExecutionInstanceChipSnippetRealtimeExecutor::wait_before_realtime +
			      ExecutionInstanceChipSnippetRealtimeExecutor::wait_after_realtime) +
			     periodic_cadc_fpga_wait_clock_cycles -
			     (periodic_cadc_ppu_wait_clock_cycles /
			      2 /* 250MHz vs. 125 MHz PPU vs. FPGA clock */)) /
			    approx_sample_duration;
			// add one sample as constant margin for error
			estimated_cadc_recording_size += 1;
			// cap at maximal possible amount of samples
			size_t const maximal_size = overall_ppu_usage.has_periodic_cadc_readout_on_dram
			                                ? num_cadc_samples_in_extmem_dram
			                                : num_cadc_samples_in_extmem;
			if (estimated_cadc_recording_size > maximal_size) {
				LOG4CXX_WARN(
				    logger, "It is estimated, that the CADC will measure "
				                << estimated_cadc_recording_size << " samples , but only the first "
				                << maximal_size << " can be recorded.");
				estimated_cadc_recording_size = maximal_size;
			}
			ppu_program_generator.num_periodic_cadc_samples = estimated_cadc_recording_size;
			// TODO: move to execution::detail namespace
			CachingCompiler compiler;
			auto program = compiler.compile(ppu_program_generator.done());

			result.internal.resize(
			    snippet_count, {program.memory.internal, program.memory.internal});
			result.external = std::move(program.memory.external);
			result.external_dram = std::move(program.memory.external_dram);
			result.symbols = std::move(program.symbols);

			ppu_neuron_reset_mask_coord =
			    std::get<PPUMemoryBlockOnPPU>(result.symbols->at("neuron_reset_mask").coordinate);
			ppu_location_coord =
			    std::get<PPUMemoryBlockOnPPU>(result.symbols->at("ppu").coordinate).toMin();
			ppu_status_coord =
			    std::get<PPUMemoryBlockOnPPU>(result.symbols->at("status").coordinate);
			ppu_stopped_coord =
			    std::get<PPUMemoryBlockOnPPU>(result.symbols->at("stopped").coordinate);
		}
		LOG4CXX_TRACE(logger, "Generated PPU program in " << ppu_timer.print() << ".");

		for (size_t i = 0; i < snippet_count; i++) {
			for (auto const ppu : iter_all<PPUOnDLS>()) {
				// set neuron reset mask
				halco::common::typed_array<int8_t, NeuronColumnOnDLS> values;
				for (auto const col : iter_all<NeuronColumnOnDLS>()) {
					values[col] =
					    ppu_usages[i].enabled_neuron_resets
					        [AtomicNeuronOnDLS(col, ppu.toNeuronRowOnDLS()).toNeuronResetOnDLS()];
				}
				auto const neuron_reset_mask = to_vector_unit_row(values);
				result.internal[i][ppu].set_subblock(
				    ppu_neuron_reset_mask_coord.toMin(), neuron_reset_mask);
				// set PPU location
				result.internal[i][ppu].at(ppu_location_coord) =
				    PPUMemoryWord(PPUMemoryWord::Value(ppu.value()));
			}

			// inject playback-hook PPU symbols to be overwritten
			for (auto const& [name, memory_config] :
			     m_hooks.chips.at(m_chip_on_connection).write_ppu_symbols) {
				if (!result.symbols) {
					throw std::runtime_error("Provided PPU symbols but no PPU program is present.");
				}
				if (!result.symbols->contains(name)) {
					throw std::runtime_error(
					    "Provided unknown symbol name via ExecutionInstanceHooks.");
				}
				std::visit(
				    hate::overloaded{
				        [&](PPUMemoryBlockOnPPU const& coordinate) {
					        for (auto const& [hemisphere, memory] : std::get<
					                 std::map<HemisphereOnDLS, haldls::vx::v3::PPUMemoryBlock>>(
					                 memory_config)) {
						        result.internal[i][hemisphere.toPPUOnDLS()].set_subblock(
						            coordinate.toMin(), memory);
					        }
				        },
				        [&](ExternalPPUMemoryBlockOnFPGA const& coordinate) {
					        assert(result.external);
					        // only apply once
					        if (i > 0) {
						        return;
					        }
					        result.external->set_subblock(
					            coordinate.toMin().value(),
					            std::get<lola::vx::v3::ExternalPPUMemoryBlock>(memory_config));
				        },
				        [&](ExternalPPUDRAMMemoryBlockOnFPGA const& coordinate) {
					        assert(result.external_dram);
					        // only apply once
					        if (i > 0) {
						        return;
					        }
					        result.external_dram->set_subblock(
					            coordinate.toMin().value(),
					            std::get<lola::vx::v3::ExternalPPUDRAMMemoryBlock>(memory_config));
				        }},
				    result.symbols->at(name).coordinate);
			}
		}
	}
	return result;
}

} // namespace grenade::vx::execution::detail
