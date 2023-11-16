#pragma once
#include "grenade/vx/signal_flow/event.h"
#include "haldls/vx/v3/timer.h"
#include "hate/visibility.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/v3/absolute_time_playback_program_builder.h"

namespace grenade::vx::execution::detail::generator {

/**
 * Generator for a playback program snippet from a timed spike sequence.
 */
class TimedSpikeToChipSequence
{
public:
	TimedSpikeToChipSequence(signal_flow::TimedSpikeToChipSequence const& values) : m_values(values)
	{}

	typedef haldls::vx::v3::Timer::Value Result;
	typedef stadls::vx::v3::AbsoluteTimePlaybackProgramBuilder Builder;

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

private:
	friend auto stadls::vx::generate<TimedSpikeToChipSequence>(TimedSpikeToChipSequence const&);

	signal_flow::TimedSpikeToChipSequence const& m_values;
};

} // namespace grenade::vx::execution::detail::generator
