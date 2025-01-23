#pragma once
#include "grenade/vx/ppu/detail/status.h"
#include "grenade/vx/signal_flow/output_data.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "haldls/vx/v3/timer.h"
#include "hate/nil.h"
#include "hate/visibility.h"
#include "lola/vx/v3/ppu.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/v3/absolute_time_playback_program_builder.h"
#include "stadls/vx/v3/absolute_time_playback_program_container_ticket.h"
#include "stadls/vx/v3/container_ticket.h"
#include "stadls/vx/v3/playback_program_builder.h"

namespace grenade::vx::execution::detail::generator {

/**
 * Generator for a playback program snippet from inserting a PPU command and blocking until
 * completion.
 */
struct PPUCommand
{
	/**
	 * Result containing tickets to check for expected status before sending the new status command.
	 */
	struct Result
	{
		haldls::vx::v3::Timer::Value duration;
		halco::common::typed_array<
		    stadls::vx::v3::AbsoluteTimePlaybackProgramContainerTicket,
		    halco::hicann_dls::vx::v3::PPUOnDLS>
		    previous_status;
		grenade::vx::ppu::detail::Status expected_previous_status;

		/**
		 * Evaluate whether read-out status matches expectation.
		 * Issues a warning if they don't match.
		 */
		void evaluate() const;
	};
	typedef stadls::vx::v3::AbsoluteTimePlaybackProgramBuilder Builder;

	/**
	 * Construct blocking PPU command.
	 * @param coord PPU memory location at chich to place the command and to poll
	 * @param status Command to place
	 * @param expected_previous_status Expectation of status before issuing the new command.
	 */
	PPUCommand(
	    halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU const& coord,
	    grenade::vx::ppu::detail::Status const status,
	    grenade::vx::ppu::detail::Status const expected_previous_status =
	        grenade::vx::ppu::detail::Status::idle) :
	    m_coord(coord), m_status(status), m_expected_previous_status(expected_previous_status)
	{}

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<PPUCommand>(PPUCommand const&);

private:
	halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU m_coord;
	grenade::vx::ppu::detail::Status m_status;
	grenade::vx::ppu::detail::Status m_expected_previous_status;
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
	PPUStop(
	    halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU const& coord,
	    halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU const& stopped_coord) :
	    m_coord(coord), m_stopped_coord(stopped_coord)
	{}

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<PPUStop>(PPUStop const&);

private:
	halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU m_coord;
	halco::hicann_dls::vx::v3::PPUMemoryWordOnPPU m_stopped_coord;
};


/**
 * Generator for a playback program snippet for reads of PPU symbols requested in hooks.
 */
struct PPUReadHooks
{
	struct Result
	{
		std::map<
		    std::string,
		    std::variant<
		        std::map<
		            halco::hicann_dls::vx::v3::HemisphereOnDLS,
		            stadls::vx::v3::ContainerTicket>,
		        stadls::vx::v3::ContainerTicket>>
		    tickets;

		signal_flow::OutputData::ReadPPUSymbols::value_type::mapped_type evaluate() const;
	};

	typedef stadls::vx::v3::PlaybackProgramBuilder Builder;

	/**
	 * Construct generator for read hooks.
	 * @param symbol_names PPU symbol names to read out
	 * @param symbols PPU symbols to use for location lookup
	 */
	PPUReadHooks(
	    std::set<std::string> const& symbol_names,
	    lola::vx::v3::PPUElfFile::symbols_type const& symbols) :
	    m_symbol_names(symbol_names), m_symbols(symbols)
	{}

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<PPUReadHooks>(PPUReadHooks const&);

private:
	std::set<std::string> const& m_symbol_names;
	lola::vx::v3::PPUElfFile::symbols_type const& m_symbols;
};

} // namespace grenade::vx::execution::detail::generator
