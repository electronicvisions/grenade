#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/detail/execution_instance_chip_ppu_program_compiler.h"
#include "grenade/vx/execution/detail/execution_instance_snippet_realtime_executor.h"
#include "grenade/vx/execution/detail/generator/health_info.h"
#include "grenade/vx/signal_flow/execution_instance_hooks.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input_data.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "haldls/vx/v3/ppu.h"
#include "hate/visibility.h"
#include "lola/vx/v3/ppu.h"
#include <vector>

namespace grenade::vx::execution::backend {
struct PlaybackProgram;
} // namespace grenade::vx::execution::backend

namespace grenade::vx::execution::detail {

struct ExecutionInstanceRealtimeExecutor
{
	struct Program
	{
		std::vector<ExecutionInstanceSnippetRealtimeExecutor::Ret> realtime_columns;

		struct Chip
		{
			std::vector<stadls::vx::v3::PlaybackProgramBuilder> cadc_finalize_builders;
			bool uses_madc;
		};

		std::map<common::ChipOnConnection, Chip> chips;
	};

	struct PostProcessor
	{
		grenade::common::ExecutionInstanceID execution_instance;
		std::vector<ExecutionInstanceSnippetRealtimeExecutor> snippet_executors;

		struct Chip
		{
			std::vector<halco::common::typed_array<
			    std::optional<stadls::vx::v3::ContainerTicket>,
			    halco::hicann_dls::vx::v3::PPUOnDLS>>
			    cadc_readout_tickets;
			std::vector<std::optional<ExecutionInstanceNode::PeriodicCADCReadoutTimes>>
			    cadc_readout_time_information;
		};

		std::map<common::ChipOnConnection, Chip> chips;

		std::vector<ExecutionInstanceSnippetRealtimeExecutor::Result> operator()(
		    backend::PlaybackProgram const& playback_program) SYMBOL_VISIBLE;
	};

	/**
	 * Construct executor.
	 * @param graphs Graphs to use for locality and property lookup
	 * @param input_data Input datas to use
	 * @param output_data Output datas to use
	 * @param ppu_program PPU program to look up symbols and instructions
	 * @param chips_on_connection Chip identifiers on connection to use
	 * @param execution_instance Local execution instance to execute
	 */
	ExecutionInstanceRealtimeExecutor(
	    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
	    std::vector<signal_flow::InputDataSnippet> const& input_data,
	    std::vector<signal_flow::OutputDataSnippet>& output_data,
	    std::map<common::ChipOnConnection, ExecutionInstanceChipPPUProgramCompiler::Result> const&
	        ppu_program,
	    std::vector<common::ChipOnConnection> chips_on_connection,
	    grenade::common::ExecutionInstanceID const& execution_instance) SYMBOL_VISIBLE;

	std::pair<Program, PostProcessor> operator()() const SYMBOL_VISIBLE;

private:
	std::vector<std::reference_wrapper<signal_flow::Graph const>> const& m_graphs;
	std::vector<signal_flow::InputDataSnippet> const& m_input_data;
	std::vector<signal_flow::OutputDataSnippet>& m_output_data;
	std::map<common::ChipOnConnection, ExecutionInstanceChipPPUProgramCompiler::Result> const&
	    m_ppu_program;
	std::vector<common::ChipOnConnection> m_chips_on_connection;
	grenade::common::ExecutionInstanceID m_execution_instance;
};

} // namespace grenade::vx::execution::detail
