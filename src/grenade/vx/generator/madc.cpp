#include "grenade/vx/generator/madc.h"

#include "halco/hicann-dls/vx/v2/barrier.h"
#include "halco/hicann-dls/vx/v2/madc.h"
#include "haldls/vx/v2/barrier.h"
#include "haldls/vx/v2/madc.h"
#include "stadls/vx/v2/playback_program_builder.h"

namespace grenade::vx::generator {

using namespace haldls::vx::v2;
using namespace stadls::vx::v2;
using namespace halco::hicann_dls::vx::v2;

PlaybackGeneratorReturn<MADCArm::Result> MADCArm::generate() const
{
	PlaybackProgramBuilder builder;

	MADCControl config;
	config.set_enable_power_down_after_sampling(true);
	config.set_start_recording(false);
	config.set_wake_up(true);
	config.set_enable_pre_amplifier(true);
	config.set_enable_continuous_sampling(true);
	builder.write(MADCControlOnDLS(), config);
	builder.block_until(BarrierOnFPGA(), Barrier::omnibus);

	return {std::move(builder), hate::Nil{}};
}


PlaybackGeneratorReturn<MADCStart::Result> MADCStart::generate() const
{
	PlaybackProgramBuilder builder;

	MADCControl config;
	config.set_enable_power_down_after_sampling(true);
	config.set_start_recording(true);
	config.set_wake_up(false);
	config.set_enable_pre_amplifier(true);
	config.set_enable_continuous_sampling(true);
	builder.write(MADCControlOnDLS(), config);
	builder.block_until(BarrierOnFPGA(), Barrier::omnibus);

	return {std::move(builder), hate::Nil{}};
}


PlaybackGeneratorReturn<MADCStop::Result> MADCStop::generate() const
{
	PlaybackProgramBuilder builder;

	MADCControl config;
	config.set_enable_power_down_after_sampling(true);
	config.set_enable_continuous_sampling(true);
	config.set_stop_recording(true);
	builder.write(MADCControlOnDLS(), config);

	return {std::move(builder), hate::Nil{}};
}

} // namespace grenade::vx::generator
