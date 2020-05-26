#pragma once
#include <variant>
#include <vector>

#include "haldls/vx/event.h"
#include "haldls/vx/timer.h"
#include "hate/visibility.h"

namespace grenade::vx {

/**
 * Timed to-chip spike.
 */
struct TimedSpikeEvent
{
	typedef haldls::vx::Timer::Value Time;
	/** Time value when to emit spike. */
	Time time;

	typedef std::variant<
	    haldls::vx::SpikePack1ToChip,
	    haldls::vx::SpikePack2ToChip,
	    haldls::vx::SpikePack3ToChip>
	    Payload;

	/** Spike payload data. */
	Payload payload;

	bool operator==(TimedSpikeEvent const& other) const SYMBOL_VISIBLE;
	bool operator!=(TimedSpikeEvent const& other) const SYMBOL_VISIBLE;
};

/** Sequence of timed to-chip spike events. */
typedef std::vector<TimedSpikeEvent> TimedSpikeEventSequence;

} // namespace grenade::vx
