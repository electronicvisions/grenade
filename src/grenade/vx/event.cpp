#include "grenade/vx/event.h"

#include "stadls/vx/v2/playback_program_builder.h"

namespace grenade::vx {

TimedSpike::TimedSpike(Time const& time, Payload const& payload) : time(time), payload(payload) {}

bool TimedSpike::operator==(TimedSpike const& other) const
{
	return time == other.time && payload == other.payload;
}

bool TimedSpike::operator!=(TimedSpike const& other) const
{
	return !(*this == other);
}

stadls::vx::v2::PlaybackGeneratorReturn<TimedSpikeSequenceGenerator::Result>
TimedSpikeSequenceGenerator::generate() const
{
	using namespace haldls::vx::v2;
	using namespace stadls::vx::v2;
	using namespace halco::hicann_dls::vx::v2;

	PlaybackProgramBuilder builder;

	builder.write(TimerOnDLS(), Timer());
	TimedSpike::Time current_time(0);
	for (auto const& event : m_values) {
		if (event.time == current_time + 1) {
			current_time = event.time;
		} else if (event.time > current_time) {
			current_time = event.time;
			builder.block_until(TimerOnDLS(), current_time);
		}
		std::visit(
		    [&](auto const& p) {
			    typedef hate::remove_all_qualifiers_t<decltype(p)> container_type;
			    builder.write(typename container_type::coordinate_type(), p);
		    },
		    event.payload);
	}

	return {std::move(builder), hate::Nil{}};
}

} // namespace grenade::vx
