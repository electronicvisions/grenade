#pragma once
#include "grenade/common/linked_topology.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/execution_instance_hooks.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"
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
	 * @param topologies Topologies to use
	 * @param execution_instance_vertex_descriptors Vertex descriptors per realtime snippet of
	 * execution instance to visit
	 * @param input_data Input data to use
	 * @param hooks Execution instance hooks to use
	 * @param chip_on_connection Chip identifier on connection to use
	 */
	ExecutionInstanceChipPPUProgramCompiler(
	    std::vector<std::shared_ptr<grenade::common::LinkedTopology>> const& topologies,
	    std::vector<grenade::common::VertexOnTopology> const& execution_instance_vertex_descriptors,
	    std::vector<std::reference_wrapper<grenade::common::InputData const>> const& input_data,
	    ExecutionInstanceHooks const& hooks,
	    common::ChipOnConnection const& chip_on_connection) SYMBOL_VISIBLE;

	Result operator()() const SYMBOL_VISIBLE;

private:
	std::vector<std::shared_ptr<grenade::common::LinkedTopology>> const& m_topologies;
	std::vector<grenade::common::VertexOnTopology> const& m_execution_instance_vertex_descriptors;
	std::vector<std::reference_wrapper<grenade::common::InputData const>> const& m_input_data;
	ExecutionInstanceHooks const& m_hooks;
	common::ChipOnConnection m_chip_on_connection;
};

} // namespace grenade::vx::execution::detail
