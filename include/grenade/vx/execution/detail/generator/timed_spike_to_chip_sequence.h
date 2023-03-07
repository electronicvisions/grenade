#pragma once
#include "grenade/vx/signal_flow/event.h"
#include "hate/nil.h"
#include "hate/visibility.h"
#include "stadls/vx/v3/playback_generator.h"

namespace grenade::vx::execution::detail::generator {

/**
 * Generator for a playback program snippet from a timed spike sequence.
 */
class TimedSpikeToChipSequence
{
public:
	TimedSpikeToChipSequence(signal_flow::TimedSpikeToChipSequence const& values) : m_values(values)
	{}

	typedef hate::Nil Result;
	typedef stadls::vx::v3::PlaybackProgramBuilder Builder;

protected:
	stadls::vx::v3::PlaybackGeneratorReturn<Result> generate() const SYMBOL_VISIBLE;

private:
	friend auto stadls::vx::generate<TimedSpikeToChipSequence>(TimedSpikeToChipSequence const&);

	signal_flow::TimedSpikeToChipSequence const& m_values;
};

} // namespace grenade::vx::execution::detail::generator