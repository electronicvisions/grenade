#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/detail/execution_instance_chip_ppu_program_compiler.h"
#include "grenade/vx/execution/detail/execution_instance_realtime_executor.h"
#include "grenade/vx/execution/detail/execution_instance_snippet_realtime_executor.h"
#include "grenade/vx/execution/detail/generator/health_info.h"
#include "grenade/vx/execution/execution_instance_hooks.h"
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

struct ExecutionInstanceExecutor
{
	struct PostProcessor
	{
		grenade::common::ExecutionInstanceID execution_instance;

		struct Chip
		{
			std::vector<generator::PPUReadHooks::Result> ppu_read_hooks_results;
			std::vector<generator::HealthInfo::Result> health_info_results_pre;
			std::vector<generator::HealthInfo::Result> health_info_results_post;
		};

		std::map<common::ChipOnConnection, Chip> chips;

		ExecutionInstanceRealtimeExecutor::PostProcessor realtime;

		signal_flow::OutputData operator()(backend::PlaybackProgram const& playback_program)
		    SYMBOL_VISIBLE;
	};

	/**
	 * Construct executor.
	 * @param graphs Graphs to use for locality and property lookup
	 * @param input_data Input datas to use
	 * @param output_data Output datas to use
	 * @param configs Chip configuration to use
	 * @param hooks Execution instance hooks to use
	 * @param chips_on_connection Chip identifiers on connection to use
	 * @param execution_instance Local execution instance to execute
	 */
	ExecutionInstanceExecutor(
	    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
	    signal_flow::InputData const& input_data,
	    signal_flow::OutputData& output_data,
	    std::vector<std::map<
	        common::ChipOnConnection,
	        std::reference_wrapper<lola::vx::v3::Chip const>>> const& configs,
	    ExecutionInstanceHooks& hooks,
	    std::vector<common::ChipOnConnection> const& chips_on_connection,
	    grenade::common::ExecutionInstanceID const& execution_instance) SYMBOL_VISIBLE;

	std::pair<backend::PlaybackProgram, PostProcessor> operator()() const SYMBOL_VISIBLE;

private:
	std::vector<std::reference_wrapper<signal_flow::Graph const>> const& m_graphs;
	signal_flow::InputData const& m_input_data;
	signal_flow::OutputData& m_output_data;
	std::vector<
	    std::map<common::ChipOnConnection, std::reference_wrapper<lola::vx::v3::Chip const>>> const&
	    m_configs;
	ExecutionInstanceHooks& m_hooks;
	std::vector<common::ChipOnConnection> m_chips_on_connection;
	grenade::common::ExecutionInstanceID m_execution_instance;
};

} // namespace grenade::vx::execution::detail
