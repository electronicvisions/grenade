#pragma once
#include "grenade/vx/common/timed_data.h"
#include "halco/hicann-dls/vx/v3/event.h"
#include "haldls/vx/v3/event.h"
#include "hate/visibility.h"
#include <variant>
#include <vector>

namespace grenade::vx::signal_flow {

/** To-chip spike. */
typedef std::variant<
    haldls::vx::v3::SpikePack1ToChip,
    haldls::vx::v3::SpikePack2ToChip,
    haldls::vx::v3::SpikePack3ToChip>
    SpikeToChip;

/** From-chip spike. */
typedef halco::hicann_dls::vx::v3::SpikeLabel SpikeFromChip;

/** From-chip MADC sample. */
typedef haldls::vx::v3::MADCSampleFromChip::Value MADCSampleFromChip;


/** Time-annotated to-chip spike. */
typedef common::TimedData<SpikeToChip> TimedSpikeToChip;

/** Time-annotated from-chip spike. */
typedef common::TimedData<SpikeFromChip> TimedSpikeFromChip;

/** Time-annotated from-chip MADC sample. */
typedef common::TimedData<MADCSampleFromChip> TimedMADCSampleFromChip;


/** Sequence of timed to-chip spike events. */
typedef std::vector<TimedSpikeToChip> TimedSpikeToChipSequence;

/** Sequence of time-annotated from-chip spike events. */
typedef std::vector<TimedSpikeFromChip> TimedSpikeFromChipSequence;

/** Sequence of time-annotated from-chip MADC events. */
typedef std::vector<TimedMADCSampleFromChip> TimedMADCSampleFromChipSequence;

} // namespace grenade::vx::signal_flow
