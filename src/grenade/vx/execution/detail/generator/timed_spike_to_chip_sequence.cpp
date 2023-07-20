#include "grenade/vx/execution/detail/generator/timed_spike_to_chip_sequence.h"

#include "halco/hicann-dls/vx/v3/timing.h"
#include "haldls/vx/v3/timer.h"
#include <log4cxx/logger.h>

namespace grenade::vx::execution::detail::generator {

stadls::vx::
    PlaybackGeneratorReturn<TimedSpikeToChipSequence::Builder, TimedSpikeToChipSequence::Result>
    TimedSpikeToChipSequence::generate() const
{
	Builder builder;

	for (auto const& event : m_values) {
		std::visit(
		    [&](auto const& p) {
			    typedef hate::remove_all_qualifiers_t<decltype(p)> container_type;
			    builder.write(
			        event.time.toTimerOnFPGAValue(), typename container_type::coordinate_type(), p);
		    },
		    event.data);
	}

	return {std::move(builder), hate::Nil{}};
}

} // namespace grenade::vx::execution::detail::generator
