#include "grenade/vx/execution/detail/generator/capmem.h"

#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/timer.h"

namespace grenade::vx::execution::detail::generator {

using namespace haldls::vx::v3;
using namespace stadls::vx::v3;
using namespace halco::hicann_dls::vx::v3;

stadls::vx::PlaybackGeneratorReturn<CapMemSettlingWait::Builder, CapMemSettlingWait::Result>
CapMemSettlingWait::generate() const
{
	Builder builder;

	builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);
	builder.write(TimerOnDLS(), haldls::vx::v3::Timer());
	haldls::vx::v3::Timer::Value const capmem_settling_time(
	    100000 * haldls::vx::v3::Timer::Value::fpga_clock_cycles_per_us);
	builder.block_until(TimerOnDLS(), capmem_settling_time);

	return {std::move(builder), {}};
}

} // namespace grenade::vx::execution::detail::generator
