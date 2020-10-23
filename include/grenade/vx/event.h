#pragma once
#include <variant>
#include <vector>

#include "haldls/vx/v2/event.h"
#include "haldls/vx/v2/timer.h"
#include "hate/visibility.h"

namespace grenade::vx {

/**
 * Timed to-chip spike.
 */
struct TimedSpike
{
	typedef haldls::vx::v2::Timer::Value Time;
	/** Time value when to emit spike. */
	Time time;

	typedef std::variant<
	    haldls::vx::v2::SpikePack1ToChip,
	    haldls::vx::v2::SpikePack2ToChip,
	    haldls::vx::v2::SpikePack3ToChip>
	    Payload;

	/** Spike payload data. */
	Payload payload;

	bool operator==(TimedSpike const& other) const SYMBOL_VISIBLE;
	bool operator!=(TimedSpike const& other) const SYMBOL_VISIBLE;
};

/** Sequence of timed to-chip spike events. */
typedef std::vector<TimedSpike> TimedSpikeSequence;

/** Sequence of time-annotated from-chip spike events. */
typedef std::vector<haldls::vx::v2::SpikeFromChip> TimedSpikeFromChipSequence;

/** Sequence of time-annotated from-chip MADC events. */
typedef std::vector<haldls::vx::v2::MADCSampleFromChip> TimedMADCSampleFromChipSequence;

} // namespace grenade::vx
