#include "grenade/vx/execution/detail/execution_instance_executor.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/backend/playback_program.h"
#include "grenade/vx/execution/detail/execution_instance_chip_snippet_config_visitor.h"
#include "grenade/vx/execution/detail/execution_instance_chip_snippet_ppu_usage_visitor.h"
#include "grenade/vx/execution/detail/generator/madc.h"
#include "grenade/vx/execution/detail/ppu_program_generator.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/execution_instance_global.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/detail/status.h"
#include "grenade/vx/signal_flow/vertex/chip.h"
#include "grenade/vx/signal_flow/vertex/pad_readout.h"
#include "halco/hicann-dls/vx/v3/readout.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/padi.h"
#include "haldls/vx/v3/readout.h"
#include "hate/timer.h"
#include "hate/type_index.h"
#include "hate/type_traits.h"
#include "hate/variant.h"
#include "hxcomm/common/hwdb_entry.h"
#include "lola/vx/ppu.h"
#include "stadls/vx/constants.h"
#include "stadls/vx/v3/playback_program_builder.h"
#include <filesystem>
#include <functional>
#include <iterator>
#include <boost/range/combine.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx::execution::detail {

std::vector<grenade::common::OutputData> ExecutionInstanceExecutor::PostProcessor::operator()(
    backend::PlaybackProgram&& playback_program)
{
	auto logger =
	    log4cxx::Logger::getLogger("grenade.ExecutionInstanceExecutor.Program.PostProcessor");

	size_t const num_realtime_snippets = realtime.snippet_executors.size();
	std::vector<grenade::common::OutputData> results(num_realtime_snippets);
	std::vector<network::abstract::ExecutionInstanceGlobal> results_global(num_realtime_snippets);

	for (auto const& [chip_on_connection, chip] : chips) {
		for (size_t i = 0; i < num_realtime_snippets; i++) {
			// add pre-execution config to result data map
			results_global.at(i).pre_execution_chips.emplace(
			    chip_on_connection,
			    std::visit(
			        [](auto&& system) -> lola::vx::v3::Chip { return std::move(system.chip); },
			        playback_program.chips.at(chip_on_connection).system_configs.at(i)));
		}
	}

	auto realtime_results = realtime(std::move(playback_program));

	for (size_t i = 0; auto& result : realtime_results) {
		results.at(i).merge(std::move(result.data));
		i++;
	}

	signal_flow::ExecutionHealthInfo::ExecutionInstance execution_health_info;
	for (auto const& [chip_on_connection, chip] : chips) {
		for (auto const& result : realtime_results) {
			results_global.at(num_realtime_snippets - 1).realtime_duration[chip_on_connection] +=
			    result.total_realtime_duration.at(chip_on_connection);
		}

		// extract PPU read hooks
		results_global.at(num_realtime_snippets - 1)
		    .read_ppu_symbols.resize(chip.ppu_read_hooks_results.size());
		for (size_t b = 0; b < chip.ppu_read_hooks_results.size(); ++b) {
			results_global.at(num_realtime_snippets - 1)
			    .read_ppu_symbols.at(b)[chip_on_connection] =
			    chip.ppu_read_hooks_results.at(b).evaluate();
		}

		for (size_t i = 0; i < chip.health_info_results_pre.size(); ++i) {
			execution_health_info.chips[chip_on_connection] +=
			    (chip.health_info_results_post.at(i).get_execution_health_info() -=
			     chip.health_info_results_pre.at(i).get_execution_health_info());
		}
	}

	// annotate execution health info
	results_global.at(num_realtime_snippets - 1).execution_health_info = execution_health_info;

	for (size_t i = 0; i < num_realtime_snippets; i++) {
		results.at(i).execution_instances.set(execution_instance, results_global.at(i));
	}

	return results;
}


ExecutionInstanceExecutor::ExecutionInstanceExecutor(
    std::vector<std::shared_ptr<grenade::common::LinkedTopology>> const& topologies,
    std::vector<grenade::common::VertexOnTopology> const& execution_instance_vertex_descriptors,
    std::vector<grenade::common::OutputData>& output_data,
    std::vector<std::reference_wrapper<grenade::common::InputData const>> const& input_data,
    ExecutionInstanceHooks& hooks,
    std::vector<common::ChipOnConnection> const& chips_on_connection) :
    m_topologies(topologies),
    m_execution_instance_vertex_descriptors(execution_instance_vertex_descriptors),
    m_output_data(output_data),
    m_input_data(input_data),
    m_hooks(hooks),
    m_chips_on_connection(chips_on_connection)
{
}

std::pair<backend::PlaybackProgram, ExecutionInstanceExecutor::PostProcessor>
ExecutionInstanceExecutor::operator()(
    std::map<common::ChipOnConnection, hxcomm::HwdbEntry> const& chip_hwdb_entries) const
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceExecutor");

	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace lola::vx::v3;
	using namespace haldls::vx::v3;
	using namespace stadls::vx::v3;

	assert(
	    (std::set<size_t>{m_output_data.size(), m_topologies.size(), m_input_data.size()}.size() ==
	     1));
	size_t const snippet_count = m_output_data.size();

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

		playback_program.chips.emplace(chip_on_connection, backend::PlaybackProgram::Chip());

		playback_program.chips.at(chip_on_connection).has_hooks_around_realtime =
		    has_hook_around_realtime;
		playback_program.chips.at(chip_on_connection).pre_initial_config_hook =
		    std::move(m_hooks.chips.at(chip_on_connection).pre_static_config);

		playback_program.chips.at(chip_on_connection).system_configs.reserve(snippet_count);
		for (size_t j = 0; j < snippet_count; j++) {
			if (std::holds_alternative<hwdb4cpp::HXCubeSetupEntry>(
			        chip_hwdb_entries.at(chip_on_connection))) {
				playback_program.chips.at(chip_on_connection)
				    .system_configs.emplace_back(lola::vx::v3::ChipAndSinglechipFPGA());
			} else if (
			    std::holds_alternative<hwdb4cpp::JboaSetupEntry>(
			        chip_hwdb_entries.at(chip_on_connection)) ||
			    std::holds_alternative<hxcomm::ZeroMockEntry>(
			        chip_hwdb_entries.at(chip_on_connection))) {
				playback_program.chips.at(chip_on_connection)
				    .system_configs.emplace_back(lola::vx::v3::ChipAndMultichipJboaLeafFPGA());
			} else {
				throw std::logic_error("Invalid hwdb entry.");
			}
			for (auto const& inter_graph_hyper_edge_descriptor :
			     m_topologies.at(j)->inter_graph_hyper_edges_by_linked(
			         m_execution_instance_vertex_descriptors.at(j))) {
				for (auto const& vertex_descriptor :
				     m_topologies.at(j)->references(inter_graph_hyper_edge_descriptor)) {
					if (auto const chip = dynamic_cast<signal_flow::vertex::Chip const*>(
					        &m_topologies.at(j)->get_reference().get(vertex_descriptor));
					    chip) {
						if (chip->chip_on_connection == chip_on_connection) {
							if (std::holds_alternative<hwdb4cpp::HXCubeSetupEntry>(
							        chip_hwdb_entries.at(chip_on_connection))) {
								playback_program.chips[chip_on_connection].system_configs.at(j) =
								    lola::vx::v3::ChipAndSinglechipFPGA(
								        dynamic_cast<
								            signal_flow::vertex::Chip::Parameterization const&>(
								            m_input_data.at(j).get().ports.get(
								                grenade::common::PortOnTopology{
								                    vertex_descriptor, 0}))
								            .base);
							} else if (
							    std::holds_alternative<hwdb4cpp::JboaSetupEntry>(
							        chip_hwdb_entries.at(chip_on_connection)) ||
							    std::holds_alternative<hxcomm::ZeroMockEntry>(
							        chip_hwdb_entries.at(chip_on_connection))) {
								playback_program.chips[chip_on_connection].system_configs.at(j) =
								    lola::vx::v3::ChipAndMultichipJboaLeafFPGA(
								        dynamic_cast<
								            signal_flow::vertex::Chip::Parameterization const&>(
								            m_input_data.at(j).get().ports.get(
								                grenade::common::PortOnTopology{
								                    vertex_descriptor, 0}))
								            .base);
							} else {
								throw std::logic_error("Invalid hwdb entry.");
							}
							break;
						}
					}
				}
			}
			ExecutionInstanceChipSnippetConfigVisitor(
			    *m_topologies.at(j), m_execution_instance_vertex_descriptors.at(j),
			    m_input_data.at(j).get(), chip_on_connection)(
			    playback_program.chips.at(chip_on_connection).system_configs.at(j));
		}

		auto const ppu_program = ExecutionInstanceChipPPUProgramCompiler(
		    m_topologies, m_execution_instance_vertex_descriptors, m_input_data, m_hooks,
		    chip_on_connection)();
		ppu_program.apply(playback_program);
		ppu_programs[chip_on_connection] = ppu_program;
	}

	auto [realtime_program, realtime_post_processor] = ExecutionInstanceRealtimeExecutor(
	    m_topologies, m_execution_instance_vertex_descriptors, m_input_data, m_output_data,
	    ppu_programs, m_chips_on_connection)();

	auto const first_topology_strong_component_invariant_ptr =
	    m_topologies.at(0)
	        ->get(m_execution_instance_vertex_descriptors.at(0))
	        .get_strong_component_invariant();
	assert(first_topology_strong_component_invariant_ptr);
	auto const& execution_instance =
	    dynamic_cast<grenade::common::PartitionedVertex::StrongComponentInvariant const&>(
	        *first_topology_strong_component_invariant_ptr)
	        .execution_instance_on_executor.value();

	PostProcessor post_processor;
	post_processor.execution_instance = execution_instance;
	post_processor.realtime = std::move(realtime_post_processor);

	// are all equal, use first one
	size_t const batch_size = m_input_data.at(0).get().batch_size();

	std::optional<common::Time> inter_batch_entry_wait;
	std::optional<bool> inter_batch_entry_routing_disabled;
	for (auto const& inter_graph_hyper_edge_descriptor :
	     m_topologies.at(snippet_count - 1)
	         ->inter_graph_hyper_edges_by_linked(
	             m_execution_instance_vertex_descriptors.at(snippet_count - 1))) {
		for (auto const& vertex_descriptor :
		     m_topologies.at(snippet_count - 1)->references(inter_graph_hyper_edge_descriptor)) {
			auto const& vertex =
			    m_topologies.at(snippet_count - 1)->get_reference().get(vertex_descriptor);
			if (vertex.get_time_domain()) {
				auto const& clock_cycle_time_domain_runtimes =
				    dynamic_cast<network::abstract::ClockCycleTimeDomainRuntimes const&>(
				        m_input_data.at(snippet_count - 1)
				            .get()
				            .time_domain_runtimes.get(*vertex.get_time_domain()));
				inter_batch_entry_wait = clock_cycle_time_domain_runtimes.inter_batch_entry_wait;
				inter_batch_entry_routing_disabled =
				    clock_cycle_time_domain_runtimes.inter_batch_entry_routing_disabled;
				break;
			}
		}
	}

	// Experiment assembly
	std::vector<std::map<common::ChipOnConnection, PlaybackProgramBuilder>> assembled_builders;
	for (size_t i = 0; i < batch_size; i++) {
		assembled_builders.push_back({});

		// Find maximum pre realtime duration for syncing of multi-chip setups.
		haldls::vx::v3::Timer::Value max_pre_realtime_duration(0);
		for (auto const& chip : m_chips_on_connection) {
			max_pre_realtime_duration = std::max(
			    realtime_program.snippets[0].at(chip).realtimes[i].pre_realtime_duration,
			    max_pre_realtime_duration);
		}

		for (auto const& chip_on_connection : m_chips_on_connection) {
			auto& local_realtime_program = realtime_program.chips.at(chip_on_connection);
			auto& local_hooks = m_hooks.chips.at(chip_on_connection);
			auto& local_playback_program = playback_program.chips.at(chip_on_connection);
			AbsoluteTimePlaybackProgramBuilder program_builder;
			// This ensures that all realtime sections start at the same time, even with different
			// pre realtime durations
			haldls::vx::v3::Timer::Value config_time = max_pre_realtime_duration;

			// Neuron resets
			for (auto const coord : iter_all<CommonNeuronBackendConfigOnDLS>()) {
				auto backend = std::visit(
				    [coord](auto const& system) {
					    return system.chip.neuron_block.backends[coord];
				    },
				    local_playback_program.system_configs[0]);
				backend.set_force_reset(true);
				program_builder.write(config_time, coord, backend);
			}
			// Add additional time to reset neurons
			config_time += haldls::vx::v3::Timer::Value(125);
			for (auto const coord : iter_all<CommonNeuronBackendConfigOnDLS>()) {
				auto backend = std::visit(
				    [coord](auto const& system) {
					    return system.chip.neuron_block.backends[coord];
				    },
				    local_playback_program.system_configs[0]);
				backend.set_force_reset(false);
				program_builder.write(config_time, coord, backend);
			}
			// Add additional time to reset neurons
			config_time += haldls::vx::v3::Timer::Value(4 * 125);

			for (size_t j = 0; j < snippet_count; j++) {
				if (j > 0) {
					config_time += realtime_program.snippets[j - 1]
					                   .at(chip_on_connection)
					                   .realtimes[i]
					                   .realtime_duration;
					program_builder.write(
					    config_time, ChipOnDLS(),
					    std::visit(
					        [](auto const& system) -> lola::vx::v3::Chip const& {
						        return system.chip;
					        },
					        local_playback_program.system_configs[j]),
					    std::visit(
					        [](auto const& system) -> lola::vx::v3::Chip const& {
						        return system.chip;
					        },
					        local_playback_program.system_configs[j - 1]));
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
			AbsoluteTimePlaybackProgramBuilder inside_realtime_hook;
			if (i < batch_size - 1) {
				inside_realtime_hook.copy(local_hooks.inside_realtime);
			} else {
				inside_realtime_hook.merge(local_hooks.inside_realtime);
			}
			inside_realtime_hook += config_time;
			program_builder.merge(inside_realtime_hook);

			// reset config to initial config at end of each realtime_row if experiment has
			// multiple configs
			if (realtime_program.snippets.size() > 1) {
				config_time += realtime_program.snippets[realtime_program.snippets.size() - 1]
				                   .at(chip_on_connection)
				                   .realtimes[i]
				                   .realtime_duration;
				if (i < batch_size - 1) {
					program_builder.write(
					    config_time, ChipOnDLS(),
					    std::visit(
					        [](auto const& system) -> lola::vx::v3::Chip const& {
						        return system.chip;
					        },
					        local_playback_program.system_configs[0]),
					    std::visit(
					        [](auto const& system) -> lola::vx::v3::Chip const& {
						        return system.chip;
					        },
					        local_playback_program
					            .system_configs[local_playback_program.system_configs.size() - 1]));
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
			// For multichip execution add barrier for synchronisation
			if (std::all_of(
			        chip_hwdb_entries.begin(), chip_hwdb_entries.end(),
			        [](auto& chip_and_hwdb_entry) {
				        return std::holds_alternative<hwdb4cpp::JboaSetupEntry>(
				            chip_and_hwdb_entry.second);
			        })) {
				assembled_builder.block_until(
				    halco::hicann_dls::vx::BarrierOnFPGA(), Barrier::multi_fpga);
			}
			// append realtime section
			assembled_builder.merge_back(program_builder.done());
			// append inside_realtime_end hook
			if (i < batch_size - 1) {
				assembled_builder.copy_back(local_hooks.inside_realtime_end);
			} else {
				assembled_builder.merge_back(local_hooks.inside_realtime_end);
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
			// Inter-batch-entry procedures: disable routing and/or wait
			{
				bool disable_routing =
				    inter_batch_entry_routing_disabled ? *inter_batch_entry_routing_disabled : true;

				if (disable_routing) {
					// disable internal event routing to silence network activity and state
					for (auto const crossbar_node_coord : iter_all<CrossbarNodeOnDLS>()) {
						assembled_builder.write(crossbar_node_coord, CrossbarNode::drop_all);
					}
					assembled_builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
				}
				// Implement inter_batch_entry_wait (ensures minimal waiting time in between
				// batch entries)
				if (inter_batch_entry_wait) {
					assembled_builder.block_until(
					    TimerOnDLS(), config_time + inter_batch_entry_wait->toTimerOnFPGAValue());
				}
				if (disable_routing) {
					// re-enable internal event routing for next batch entry
					for (auto const crossbar_node_coord : iter_all<CrossbarNodeOnDLS>()) {
						assembled_builder.write(
						    crossbar_node_coord,
						    std::visit(
						        [](auto const& system) -> lola::vx::v3::Chip const& {
							        return system.chip;
						        },
						        local_playback_program.system_configs[0])
						        .crossbar.nodes[crossbar_node_coord]);
					}
					assembled_builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
				}
			}
			assembled_builders.back()[chip_on_connection] = std::move(assembled_builder);
		}
		// Append cadc_finalize_builder for periodic_cadc data readout
		size_t max_num_cadc_builders = 0;
		for (auto const& chip_on_connection : m_chips_on_connection) {
			max_num_cadc_builders = std::max(
			    max_num_cadc_builders,
			    realtime_program.chips.at(chip_on_connection).cadc_finalize_builders.at(i).size());
		}
		for (size_t c = 0; c < max_num_cadc_builders; ++c) {
			assembled_builders.push_back({});
			for (auto const& chip_on_connection : m_chips_on_connection) {
				auto& local_cadc_finalize_builders =
				    realtime_program.chips.at(chip_on_connection).cadc_finalize_builders.at(i);
				if (local_cadc_finalize_builders.size() > c) {
					assembled_builders.back()[chip_on_connection] =
					    std::move(local_cadc_finalize_builders.at(c));
				} else {
					assembled_builders.back()[chip_on_connection] = {};
				}
			}
		}
		// append ppu_finish_builders
		size_t max_num_ppu_finish_builders = 0;
		for (auto const& chip_on_connection : m_chips_on_connection) {
			size_t local_num_ppu_finish_builders = 0;
			for (size_t j = 0; j < snippet_count; j++) {
				local_num_ppu_finish_builders += realtime_program.snippets[j]
				                                     .at(chip_on_connection)
				                                     .realtimes[i]
				                                     .ppu_finish_builder.size();
			}
			max_num_ppu_finish_builders =
			    std::max(max_num_ppu_finish_builders, local_num_ppu_finish_builders);
		}
		std::map<grenade::vx::common::ChipOnConnection, std::vector<PlaybackProgramBuilder>>
		    local_ppu_finish_builders;
		for (auto const& chip_on_connection : m_chips_on_connection) {
			for (size_t j = 0; j < snippet_count; j++) {
				for (auto& builder : realtime_program.snippets[j]
				                         .at(chip_on_connection)
				                         .realtimes[i]
				                         .ppu_finish_builder) {
					local_ppu_finish_builders[chip_on_connection].push_back(std::move(builder));
				}
			}
		}
		for (size_t c = 0; c < max_num_ppu_finish_builders; ++c) {
			assembled_builders.push_back({});
			for (auto const& chip_on_connection : m_chips_on_connection) {
				auto& local_builders = local_ppu_finish_builders.at(chip_on_connection);
				if (local_builders.size() > c) {
					assembled_builders.back()[chip_on_connection] = std::move(local_builders.at(c));
				} else {
					assembled_builders.back()[chip_on_connection] = {};
				}
			}
		}
		if (i == batch_size - 1) {
			// for the last batch entry, stop the PPU
			assembled_builders.push_back({});
			for (auto const& chip_on_connection : m_chips_on_connection) {
				auto& local_playback_program = playback_program.chips.at(chip_on_connection);
				if (local_playback_program.ppu_symbols) {
					assembled_builders.back()[chip_on_connection].merge_back(
					    generate(generator::PPUStop(*local_playback_program.ppu_symbols)).builder);
				} else {
					assembled_builders.back()[chip_on_connection] = {};
				}
			}
			// for the last batch entry, append post_realtime hook
			assembled_builders.push_back({});
			for (auto const& chip_on_connection : m_chips_on_connection) {
				auto& local_hooks = m_hooks.chips.at(chip_on_connection);
				assembled_builders.back()[chip_on_connection].merge_back(local_hooks.post_realtime);
			}
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
			                post_size_to_fpga) >
			               (stadls::vx::playback_memory_size_to_fpga /
			                2 /* for each read we get one time-annotation message in addition */);
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
