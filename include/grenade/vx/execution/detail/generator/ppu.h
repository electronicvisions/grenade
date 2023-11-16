#pragma once
#include "grenade/vx/ppu/detail/status.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "haldls/vx/v3/timer.h"
#include "hate/visibility.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/v3/absolute_time_playback_program_builder.h"

namespace grenade::vx::execution::detail::generator {

/**
 * Generator for a playback program snippet from inserting a PPU command and blocking until
 * completion.
 */
struct PPUCommand
{
	typedef haldls::vx::v3::Timer::Value Result;
	typedef stadls::vx::v3::AbsoluteTimePlaybackProgramBuilder Builder;

	/**
	 * Construct blocking PPU command.
	 * @param coord PPU memory location at chich to place the command and to poll
	 * @param status Command to place
	 */
	PPUCommand(
	    halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU const& coord,
	    grenade::vx::ppu::detail::Status const status) :
	    m_coord(coord), m_status(status)
	{}

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<PPUCommand>(PPUCommand const&);

private:
	halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU m_coord;
	grenade::vx::ppu::detail::Status m_status;
};

} // namespace grenade::vx::execution::detail::generator
