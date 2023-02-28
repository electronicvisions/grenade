#include "grenade/vx/execution/detail/generator/timed_spike_sequence.h"

#include "halco/hicann-dls/vx/v3/timing.h"
#include "haldls/vx/v3/timer.h"
#include "stadls/vx/v3/playback_program_builder.h"
#include <log4cxx/logger.h>

namespace grenade::vx::execution::detail::generator {

stadls::vx::v3::PlaybackGeneratorReturn<TimedSpikeSequence::Result> TimedSpikeSequence::generate()
    const
{
	auto logger = log4cxx::Logger::getLogger("grenade.generator.TimedSpikeSequence");

	using namespace haldls::vx::v3;
	using namespace stadls::vx::v3;
	using namespace halco::hicann_dls::vx::v3;

	// approx. 10ms in biological time deemed high enough to warn the user something is wrong
	signal_flow::TimedSpike::Time const max_delay(
	    signal_flow::TimedSpike::Time::fpga_clock_cycles_per_us * 10);
	PlaybackProgramBuilder builder;

	builder.write(TimerOnDLS(), Timer());
	signal_flow::TimedSpike::Time current_time(0);
	for (auto const& event : m_values) {
		if (event.time > current_time) {
			current_time = event.time;
			builder.block_until(TimerOnDLS(), Timer::Value(current_time - 1));
		} else if (event.time + max_delay < current_time) {
			LOG4CXX_WARN(
			    logger, "To be issued event at (" << event.time << ") delayed by more than "
			                                      << max_delay << ".");
		}
		std::visit(
		    [&](auto const& p) {
			    typedef hate::remove_all_qualifiers_t<decltype(p)> container_type;
			    builder.write(typename container_type::coordinate_type(), p);
		    },
		    event.payload);
		current_time = signal_flow::TimedSpike::Time(current_time + 1);
	}

	return {std::move(builder), hate::Nil{}};
}

} // namespace grenade::vx::execution::detail::generator
