#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/execution/detail/execution_instance_ppu_program_compiler.h"
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
		std::vector<stadls::vx::v3::PlaybackProgramBuilder> cadc_finalize_builders;
		bool uses_madc;
	};

	struct PostProcessor
	{
		std::vector<ExecutionInstanceSnippetRealtimeExecutor> snippet_executors;
		grenade::common::ExecutionInstanceID execution_instance;
		std::vector<halco::common::typed_array<
		    std::optional<stadls::vx::v3::ContainerTicket>,
		    halco::hicann_dls::vx::v3::PPUOnDLS>>
		    cadc_readout_tickets;
		std::vector<std::optional<ExecutionInstanceNode::PeriodicCADCReadoutTimes>>
		    cadc_readout_time_information;

		std::vector<signal_flow::OutputData> operator()(
		    backend::PlaybackProgram const& playback_program) SYMBOL_VISIBLE;
	};

	/**
	 * Construct executor.
	 * @param graphs Graphs to use for locality and property lookup
	 * @param input_data Input datas to use
	 * @param output_data Output datas to use
	 * @param ppu_program PPU program to look up symbols and instructions
	 * @param execution_instance Local execution instance to execute
	 */
	ExecutionInstanceRealtimeExecutor(
	    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
	    std::vector<std::reference_wrapper<signal_flow::InputData const>> const& input_data,
	    std::vector<signal_flow::OutputData>& output_data,
	    ExecutionInstancePPUProgramCompiler::Result const& ppu_program,
	    grenade::common::ExecutionInstanceID const& execution_instance) SYMBOL_VISIBLE;

	std::pair<Program, PostProcessor> operator()() const SYMBOL_VISIBLE;

private:
	std::vector<std::reference_wrapper<signal_flow::Graph const>> const& m_graphs;
	std::vector<std::reference_wrapper<signal_flow::InputData const>> const& m_input_data;
	std::vector<signal_flow::OutputData>& m_output_data;
	ExecutionInstancePPUProgramCompiler::Result const& m_ppu_program;
	grenade::common::ExecutionInstanceID m_execution_instance;
};

} // namespace grenade::vx::execution::detail
