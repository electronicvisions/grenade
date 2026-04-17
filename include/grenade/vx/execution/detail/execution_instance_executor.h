#pragma once
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/backend/playback_program.h"
#include "grenade/vx/execution/detail/execution_instance_realtime_executor.h"
#include "grenade/vx/execution/detail/generator/health_info.h"
#include "grenade/vx/execution/detail/system.h"
#include "grenade/vx/execution/execution_instance_hooks.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "haldls/vx/v3/ppu.h"
#include "hate/visibility.h"
#include "hxcomm/common/hwdb_entry.h"
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
		grenade::common::ExecutionInstanceOnExecutor execution_instance;

		struct Chip
		{
			std::vector<generator::PPUReadHooks::Result> ppu_read_hooks_results;
			std::vector<generator::HealthInfo::Result> health_info_results_pre;
			std::vector<generator::HealthInfo::Result> health_info_results_post;
		};

		std::map<common::ChipOnConnection, Chip> chips;

		ExecutionInstanceRealtimeExecutor::PostProcessor realtime;

		std::vector<grenade::common::OutputData> operator()(
		    backend::PlaybackProgram&& playback_program) SYMBOL_VISIBLE;
	};

	/**
	 * Construct executor.
	 * @param topologies Topologies to use
	 * @param execution_instance_vertex_descriptors Vertex descriptors per realtime snippet of
	 * execution instance to visit
	 * @param output_data Output data to add results into
	 * @param input_data Input data to use
	 * @param hooks Execution instance hooks to use
	 * @param chips_on_connection Chip identifiers on connection to use
	 */
	ExecutionInstanceExecutor(
	    std::vector<std::shared_ptr<grenade::common::LinkedTopology>> const& topologies,
	    std::vector<grenade::common::VertexOnTopology> const& execution_instance_vertex_descriptors,
	    std::vector<grenade::common::OutputData>& output_data,
	    std::vector<std::reference_wrapper<grenade::common::InputData const>> const& input_data,
	    ExecutionInstanceHooks& hooks,
	    std::vector<common::ChipOnConnection> const& chips_on_connection) SYMBOL_VISIBLE;

	std::pair<backend::PlaybackProgram, PostProcessor> operator()(
	    std::map<common::ChipOnConnection, hxcomm::HwdbEntry> const& chip_hwdb_entries) const
	    SYMBOL_VISIBLE;

private:
	std::vector<std::shared_ptr<grenade::common::LinkedTopology>> const& m_topologies;
	std::vector<grenade::common::VertexOnTopology> const& m_execution_instance_vertex_descriptors;
	std::vector<grenade::common::OutputData>& m_output_data;
	std::vector<std::reference_wrapper<grenade::common::InputData const>> const& m_input_data;
	ExecutionInstanceHooks& m_hooks;
	std::vector<common::ChipOnConnection> m_chips_on_connection;
};

} // namespace grenade::vx::execution::detail
