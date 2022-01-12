#include "grenade/vx/generator/timed_spike_sequence.h"

#include "halco/hicann-dls/vx/v2/timing.h"
#include "haldls/vx/v2/timer.h"
#include "stadls/vx/v2/playback_program_builder.h"

namespace grenade::vx::generator {

stadls::vx::v2::PlaybackGeneratorReturn<TimedSpikeSequence::Result> TimedSpikeSequence::generate()
    const
{
	using namespace haldls::vx::v2;
	using namespace stadls::vx::v2;
	using namespace halco::hicann_dls::vx::v2;

	PlaybackProgramBuilder builder;

	builder.write(TimerOnDLS(), Timer());
	TimedSpike::Time current_time(0);
	for (auto const& event : m_values) {
		if (event.time > current_time) {
			current_time = event.time;
			builder.block_until(TimerOnDLS(), Timer::Value(current_time - 1));
		}
		std::visit(
		    [&](auto const& p) {
			    typedef hate::remove_all_qualifiers_t<decltype(p)> container_type;
			    builder.write(typename container_type::coordinate_type(), p);
		    },
		    event.payload);
		current_time = TimedSpike::Time(current_time + 1);
	}

	return {std::move(builder), hate::Nil{}};
}

} // namespace grenade::vx::generator
