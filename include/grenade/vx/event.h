#pragma once
#include "grenade/vx/genpybind.h"
#include "haldls/vx/event.h"
#include "haldls/vx/timer.h"
#include "haldls/vx/v2/event.h"
#include "haldls/vx/v2/timer.h"
#include "hate/visibility.h"
#include <iosfwd>
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

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, TimedSpike const& spike) SYMBOL_VISIBLE;
};

/** Sequence of timed to-chip spike events. */
typedef std::vector<TimedSpike> TimedSpikeSequence;

/** Sequence of time-annotated from-chip spike events. */
typedef std::vector<haldls::vx::v2::SpikeFromChip> TimedSpikeFromChipSequence;

/** Sequence of time-annotated from-chip MADC events. */
typedef std::vector<haldls::vx::v2::MADCSampleFromChip> TimedMADCSampleFromChipSequence;

template <typename T>
struct TimedData
{
	haldls::vx::v2::FPGATime fpga_time;
	haldls::vx::v2::ChipTime chip_time;
	T data;

	TimedData() = default;
	TimedData(
	    haldls::vx::v2::FPGATime const& fpga_time,
	    haldls::vx::v2::ChipTime const& chip_time,
	    T const& data);

	bool operator==(TimedData const& other) const;
	bool operator!=(TimedData const& other) const;
};

template <typename T>
using TimedDataSequence = std::vector<TimedData<T>>;

} // namespace grenade::vx

#include "grenade/vx/event.tcc"
