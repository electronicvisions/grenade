#pragma once
#include "grenade/vx/ppu/detail/status.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "haldls/vx/v3/timer.h"
#include "hate/nil.h"
#include "hate/visibility.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/v3/absolute_time_playback_program_builder.h"
#include "stadls/vx/v3/playback_program_builder.h"

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


/**
 * Generator for a playback program snippet for starting the PPUs.
 */
struct PPUStart
{
	typedef hate::Nil Result;
	typedef stadls::vx::v3::PlaybackProgramBuilder Builder;

	/**
	 * Construct blocking PPU command.
	 * @param coord PPU memory location at which to place the command
	 */
	PPUStart(halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU const& coord) : m_coord(coord) {}

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<PPUStart>(PPUStart const&);

private:
	halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU m_coord;
};


/**
 * Generator for a playback program snippet for stopping the PPUs.
 */
struct PPUStop
{
	typedef hate::Nil Result;
	typedef stadls::vx::v3::PlaybackProgramBuilder Builder;

	/**
	 * Construct blocking PPU command.
	 * @param coord PPU memory location at which to place the command
	 */
	PPUStop(halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU const& coord) : m_coord(coord) {}

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<PPUStop>(PPUStop const&);

private:
	halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU m_coord;
};

} // namespace grenade::vx::execution::detail::generator
