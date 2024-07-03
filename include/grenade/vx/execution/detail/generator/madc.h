#pragma once
#include "haldls/vx/v3/timer.h"
#include "hate/nil.h"
#include "hate/visibility.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/v3/absolute_time_playback_program_builder.h"
#include "stadls/vx/v3/playback_program_builder.h"

namespace grenade::vx::execution::detail::generator {

/**
 * Generator for a playback program snippet from arming the MADC.
 */
struct MADCArm
{
	typedef hate::Nil Result;
	typedef stadls::vx::v3::PlaybackProgramBuilder Builder;

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<MADCArm>(MADCArm const&);
};


/**
 * Generator for a playback program snippet from starting the MADC.
 */
struct MADCStart
{
	typedef haldls::vx::v3::Timer::Value Result;
	typedef stadls::vx::v3::AbsoluteTimePlaybackProgramBuilder Builder;

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<MADCStart>(MADCStart const&);
};


/**
 * Generator for a playback program snippet from stopping the MADC.
 */
struct MADCStop
{
	typedef haldls::vx::v3::Timer::Value Result;
	typedef stadls::vx::v3::AbsoluteTimePlaybackProgramBuilder Builder;

	bool enable_power_down_after_sampling{false};

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<MADCStop>(MADCStop const&);
};

} // namespace grenade::vx::execution::detail::generator
