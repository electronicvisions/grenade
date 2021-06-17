#pragma once
#include "grenade/vx/ppu/status.h"
#include "halco/hicann-dls/vx/v2/ppu.h"
#include "hate/nil.h"
#include "hate/visibility.h"
#include "stadls/vx/v2/playback_generator.h"

namespace grenade::vx::generator {

/**
 * Generator for a playback program snippet from inserting a PPU command and blocking until
 * completion.
 */
struct BlockingPPUCommand
{
	typedef hate::Nil Result;
	typedef stadls::vx::v2::PlaybackProgramBuilder Builder;

	/**
	 * Construct blocking PPU command.
	 * @param coord PPU memory location at chich to place the command and to poll
	 * @param status Command to place
	 */
	BlockingPPUCommand(
	    halco::hicann_dls::vx::v2::PPUMemoryWordOnPPU const& coord,
	    grenade::vx::ppu::Status const status) :
	    m_coord(coord), m_status(status)
	{}

protected:
	stadls::vx::v2::PlaybackGeneratorReturn<Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<BlockingPPUCommand>(BlockingPPUCommand const&);

private:
	halco::hicann_dls::vx::v2::PPUMemoryWordOnPPU m_coord;
	grenade::vx::ppu::Status m_status;
};

} // namespace grenade::vx::generator
