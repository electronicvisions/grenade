#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/common/chip_on_connection.h"
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

struct ExecutionInstanceChipPPUProgramCompiler
{
	struct Result
	{
		common::ChipOnConnection chip_on_connection;

		/**
		 * Internal memory per realtime snippet.
		 */
		std::vector<halco::common::typed_array<
		    haldls::vx::v3::PPUMemoryBlock,
		    halco::hicann_dls::vx::v3::PPUOnDLS>>
		    internal;

		std::optional<lola::vx::v3::ExternalPPUMemoryBlock> external;
		std::optional<lola::vx::v3::ExternalPPUDRAMMemoryBlock> external_dram;

		std::optional<lola::vx::v3::PPUElfFile::symbols_type> symbols;

		std::vector<std::map<signal_flow::vertex::PlasticityRule::ID, size_t>>
		    plasticity_rule_timed_recording_start_periods;

		void apply(backend::PlaybackProgram& playback_program) const SYMBOL_VISIBLE;
	};

	/**
	 * Construct compiler.
	 * @param graphs Graphs to use for locality and property lookup
	 * @param input_data Input datas to use for runtime extraction
	 * @param hooks Execution instance hooks to get PPU symbols to read and write
	 * @param execution_instance Local execution instance to compile for
	 */
	ExecutionInstanceChipPPUProgramCompiler(
	    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
	    signal_flow::InputData const& input_data,
	    ExecutionInstanceHooks const& hooks,
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::ExecutionInstanceID const& execution_instance) SYMBOL_VISIBLE;

	Result operator()() const SYMBOL_VISIBLE;

private:
	std::vector<std::reference_wrapper<signal_flow::Graph const>> const& m_graphs;
	signal_flow::InputData const& m_input_data;
	ExecutionInstanceHooks const& m_hooks;
	common::ChipOnConnection m_chip_on_connection;
	grenade::common::ExecutionInstanceID m_execution_instance;
};

} // namespace grenade::vx::execution::detail
