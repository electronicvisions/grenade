#pragma once
#include "grenade/common/input_data.h"
#include "grenade/common/output_data.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/detail/execution_instance_chip_ppu_program_compiler.h"
#include "grenade/vx/execution/detail/execution_instance_snippet_realtime_executor.h"
#include "grenade/vx/execution/detail/generator/health_info.h"
#include "grenade/vx/execution/execution_instance_hooks.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "haldls/vx/v3/ppu.h"
#include "hate/visibility.h"
#include "lola/vx/v3/ppu.h"
#include <memory>
#include <vector>

namespace grenade::vx::execution::backend {
struct PlaybackProgram;
} // namespace grenade::vx::execution::backend

namespace grenade::vx::execution::detail {

struct ExecutionInstanceRealtimeExecutor
{
	struct Program
	{
		std::vector<ExecutionInstanceSnippetRealtimeExecutor::Program> snippets;

		struct Chip
		{
			std::vector<std::vector<stadls::vx::v3::PlaybackProgramBuilder>> cadc_finalize_builders;
			bool uses_madc;
		};

		std::map<common::ChipOnConnection, Chip> chips;
	};

	struct PostProcessor
	{
		grenade::common::ExecutionInstanceOnExecutor execution_instance;
		std::vector<std::unique_ptr<ExecutionInstanceSnippetRealtimeExecutor>> snippet_executors;

		struct Chip
		{
			std::vector<halco::common::typed_array<
			    std::vector<stadls::vx::v3::ContainerTicket>,
			    halco::hicann_dls::vx::v3::PPUOnDLS>>
			    cadc_readout_tickets;
			std::vector<std::optional<ExecutionInstanceNode::PeriodicCADCReadoutTimes>>
			    cadc_readout_time_information;
		};

		std::map<common::ChipOnConnection, Chip> chips;

		std::vector<ExecutionInstanceSnippetRealtimeExecutor::Result> operator()(
		    backend::PlaybackProgram&& playback_program) SYMBOL_VISIBLE;
	};

	/**
	 * Construct executor.
	 * @param topologies Topologies to use
	 * @param execution_instance_vertex_descriptors Vertex descriptors per realtime snippet of
	 * execution instance to visit
	 * @param input_data Input data to use
	 * @param output_data Output data to add results into
	 * @param ppu_program PPU program to look up symbols and instructions
	 * @param chips_on_connection Chip identifiers on connection to use
	 */
	ExecutionInstanceRealtimeExecutor(
	    std::vector<std::shared_ptr<grenade::common::LinkedTopology>> const& topologies,
	    std::vector<grenade::common::VertexOnTopology> const& execution_instance_vertex_descriptors,
	    std::vector<std::reference_wrapper<grenade::common::InputData const>> const& input_data,
	    std::vector<grenade::common::OutputData>& output_data,
	    std::map<common::ChipOnConnection, ExecutionInstanceChipPPUProgramCompiler::Result> const&
	        ppu_program,
	    std::vector<common::ChipOnConnection> chips_on_connection) SYMBOL_VISIBLE;

	std::pair<Program, PostProcessor> operator()() const SYMBOL_VISIBLE;

private:
	std::vector<std::shared_ptr<grenade::common::LinkedTopology>> const& m_topologies;
	std::vector<grenade::common::VertexOnTopology> const& m_execution_instance_vertex_descriptors;
	std::vector<std::reference_wrapper<grenade::common::InputData const>> const& m_input_data;
	std::vector<grenade::common::OutputData>& m_output_data;
	std::map<common::ChipOnConnection, ExecutionInstanceChipPPUProgramCompiler::Result> const&
	    m_ppu_program;
	std::vector<common::ChipOnConnection> m_chips_on_connection;
};

} // namespace grenade::vx::execution::detail
