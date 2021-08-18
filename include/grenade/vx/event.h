#pragma once
#include "grenade/vx/genpybind.h"
#include "haldls/vx/event.h"
#include "haldls/vx/timer.h"
#include "haldls/vx/v2/event.h"
#include "haldls/vx/v2/timer.h"
#include "hate/visibility.h"
#include <variant>
#include <vector>

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

/**
 * Timed to-chip spike.
 */
struct GENPYBIND(visible) TimedSpike
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

	TimedSpike() = default;
	TimedSpike(Time const& time, Payload const& payload) SYMBOL_VISIBLE;

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
