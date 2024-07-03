#pragma once
#include "hate/nil.h"
#include "hate/visibility.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/v3/playback_program_builder.h"

namespace grenade::vx::execution::detail::generator {

/**
 * Generator for a playback program snippet for waiting for CapMem to settle.
 */
struct CapMemSettlingWait
{
	typedef hate::Nil Result;
	typedef stadls::vx::v3::PlaybackProgramBuilder Builder;

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<CapMemSettlingWait>(CapMemSettlingWait const&);
};

} // namespace grenade::vx::execution::detail::generator
