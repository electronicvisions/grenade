#pragma once
#include "hate/nil.h"
#include "hate/visibility.h"
#include "stadls/vx/v2/playback_generator.h"

namespace grenade::vx::generator {

/**
 * Generator for a playback program snippet from arming the MADC.
 */
struct MADCArm
{
	typedef hate::Nil Result;
	typedef stadls::vx::v2::PlaybackProgramBuilder Builder;

protected:
	/**
	 * Generate PlaybackProgramBuilder.
	 * @return PlaybackGeneratorReturn instance with sequence embodied and specified Result value
	 */
	stadls::vx::v2::PlaybackGeneratorReturn<Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<MADCArm>(MADCArm const&);
};


/**
 * Generator for a playback program snippet from starting the MADC.
 */
struct MADCStart
{
	typedef hate::Nil Result;
	typedef stadls::vx::v2::PlaybackProgramBuilder Builder;

protected:
	/**
	 * Generate PlaybackProgramBuilder.
	 * @return PlaybackGeneratorReturn instance with sequence embodied and specified Result value
	 */
	stadls::vx::v2::PlaybackGeneratorReturn<Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<MADCStart>(MADCStart const&);
};


/**
 * Generator for a playback program snippet from stopping the MADC.
 */
struct MADCStop
{
	typedef hate::Nil Result;
	typedef stadls::vx::v2::PlaybackProgramBuilder Builder;

protected:
	/**
	 * Generate PlaybackProgramBuilder.
	 * @return PlaybackGeneratorReturn instance with sequence embodied and specified Result value
	 */
	stadls::vx::v2::PlaybackGeneratorReturn<Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<MADCStop>(MADCStop const&);
};

} // namespace grenade::vx::generator
