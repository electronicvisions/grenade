#pragma once
#include "grenade/vx/signal_flow/execution_health_info.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v3/highspeed_link.h"
#include "halco/hicann-dls/vx/v3/routing_crossbar.h"
#include "haldls/vx/v3/timer.h"
#include "hate/visibility.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/v3/absolute_time_playback_program_builder.h"
#include "stadls/vx/v3/absolute_time_playback_program_container_ticket.h"

namespace grenade::vx::execution::detail::generator {

/**
 * Generator for a playback program snippet for reading out health information.
 */
struct HealthInfo
{
	struct Result
	{
		haldls::vx::v3::Timer::Value execution_duration;

		signal_flow::ExecutionHealthInfo::ExecutionInstance::Chip get_execution_health_info() const
		    SYMBOL_VISIBLE;

	private:
		friend struct HealthInfo;

		stadls::vx::v3::AbsoluteTimePlaybackProgramContainerTicket m_hicann_arq_status;
		halco::common::typed_array<
		    stadls::vx::v3::AbsoluteTimePlaybackProgramContainerTicket,
		    halco::hicann_dls::vx::v3::PhyStatusOnFPGA>
		    m_phy_status;
		halco::common::typed_array<
		    stadls::vx::v3::AbsoluteTimePlaybackProgramContainerTicket,
		    halco::hicann_dls::vx::v3::CrossbarInputOnDLS>
		    m_crossbar_input_drop_counter;
		halco::common::typed_array<
		    stadls::vx::v3::AbsoluteTimePlaybackProgramContainerTicket,
		    halco::hicann_dls::vx::v3::CrossbarOutputOnDLS>
		    m_crossbar_output_event_counter;
	};
	typedef stadls::vx::v3::AbsoluteTimePlaybackProgramBuilder Builder;

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<HealthInfo>(HealthInfo const&);
};

} // namespace grenade::vx::execution::detail::generator
