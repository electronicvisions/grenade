#include "grenade/vx/execution/detail/generator/madc.h"

#include "halco/hicann-dls/vx/v3/madc.h"
#include "haldls/vx/v3/madc.h"

namespace grenade::vx::execution::detail::generator {

using namespace haldls::vx::v3;
using namespace stadls::vx::v3;
using namespace halco::hicann_dls::vx::v3;

stadls::vx::PlaybackGeneratorReturn<MADCArm::Builder, MADCArm::Result> MADCArm::generate() const
{
	Builder builder;
	Result current_time = Result(0);

	MADCControl config;
	config.set_enable_power_down_after_sampling(false);
	config.set_start_recording(false);
	config.set_wake_up(true);
	config.set_enable_pre_amplifier(true);
	config.set_enable_continuous_sampling(true);
	builder.write(current_time, MADCControlOnDLS(), config);
	current_time += Timer::Value(2);

	return {std::move(builder), current_time};
}


stadls::vx::PlaybackGeneratorReturn<MADCArm::Builder, MADCArm::Result> MADCStart::generate() const
{
	Builder builder;
	Result current_time = Result(0);

	MADCControl config;
	config.set_enable_power_down_after_sampling(false);
	config.set_start_recording(true);
	config.set_wake_up(false);
	config.set_enable_pre_amplifier(true);
	config.set_enable_continuous_sampling(true);
	builder.write(current_time, MADCControlOnDLS(), config);
	current_time += Timer::Value(2);

	return {std::move(builder), current_time};
}


stadls::vx::PlaybackGeneratorReturn<MADCArm::Builder, MADCArm::Result> MADCStop::generate() const
{
	Builder builder;
	Result current_time = Result(0);

	MADCControl config;
	config.set_enable_power_down_after_sampling(enable_power_down_after_sampling);
	config.set_enable_continuous_sampling(true);
	config.set_stop_recording(true);
	builder.write(current_time, MADCControlOnDLS(), config);
	current_time += Timer::Value(2);

	return {std::move(builder), current_time};
}

} // namespace grenade::vx::execution::detail::generator
