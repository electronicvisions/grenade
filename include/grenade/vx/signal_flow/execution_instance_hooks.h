#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/ppu.h"
#include "lola/vx/v3/ppu.h"
#include "stadls/vx/v3/absolute_time_playback_program_builder.h"
#include "stadls/vx/v3/playback_program_builder.h"
#include <map>
#include <variant>

namespace grenade::vx::signal_flow GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

/**
 * Hooks for an execution instance for complementing the graph-based experiment notation.
 */
struct GENPYBIND(
    visible, holder_type("std::shared_ptr<grenade::vx::signal_flow::ExecutionInstanceHooks>"))
    ExecutionInstanceHooks
{
	typedef std::map<
	    std::string,
	    std::variant<
	        std::map<halco::hicann_dls::vx::v3::HemisphereOnDLS, haldls::vx::v3::PPUMemoryBlock>,
	        lola::vx::v3::ExternalPPUMemoryBlock,
	        lola::vx::v3::ExternalPPUDRAMMemoryBlock>>
	    WritePPUSymbols;

	typedef std::set<std::string> ReadPPUSymbols;

	ExecutionInstanceHooks() = default;
	ExecutionInstanceHooks(
	    stadls::vx::v3::PlaybackProgramBuilder& pre_static_config,
	    stadls::vx::v3::PlaybackProgramBuilder& pre_realtime,
	    stadls::vx::v3::PlaybackProgramBuilder& inside_realtime_begin,
	    stadls::vx::v3::AbsoluteTimePlaybackProgramBuilder& inside_realtime,
	    stadls::vx::v3::PlaybackProgramBuilder& inside_realtime_end,
	    stadls::vx::v3::PlaybackProgramBuilder& post_realtime,
	    WritePPUSymbols const& write_ppu_symbols = WritePPUSymbols(),
	    ReadPPUSymbols const& read_ppu_symbols = ReadPPUSymbols()) SYMBOL_VISIBLE;
	ExecutionInstanceHooks(ExecutionInstanceHooks const&) = delete;
	ExecutionInstanceHooks(ExecutionInstanceHooks&&) = default;
	ExecutionInstanceHooks& operator=(ExecutionInstanceHooks const&) = delete;
	ExecutionInstanceHooks& operator=(ExecutionInstanceHooks&&) = default;

	stadls::vx::v3::PlaybackProgramBuilder GENPYBIND(hidden) pre_static_config;
	stadls::vx::v3::PlaybackProgramBuilder GENPYBIND(hidden) pre_realtime;
	stadls::vx::v3::PlaybackProgramBuilder GENPYBIND(hidden) inside_realtime_begin;
	stadls::vx::v3::AbsoluteTimePlaybackProgramBuilder GENPYBIND(hidden) inside_realtime;
	stadls::vx::v3::PlaybackProgramBuilder GENPYBIND(hidden) inside_realtime_end;
	stadls::vx::v3::PlaybackProgramBuilder GENPYBIND(hidden) post_realtime;

	/**
	 * PPU symbols written once as part of the initialization process.
	 */
	WritePPUSymbols write_ppu_symbols;

	/**
	 * PPU symbols to read after every batch entry.
	 */
	ReadPPUSymbols read_ppu_symbols;
};

} // namespace grenade::vx::signal_flow
