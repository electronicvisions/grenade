#pragma once
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/signal_flow/execution_instance_hooks.h"
#include "lola/vx/v3/chip.h"
#include "lola/vx/v3/ppu.h"
#include "stadls/vx/v3/playback_program.h"
#include <optional>
#include <vector>

namespace grenade::vx::execution::backend {

/**
 * Playback program to be executed via StatefulConnection.
 */
struct PlaybackProgram
{
	struct Chip
	{
		/**
		 * Chip configs per realtime snippet.
		 */
		std::vector<lola::vx::v3::Chip> chip_configs;

		/**
		 * Initial external PPU DRAM memory config.
		 */
		std::optional<lola::vx::v3::ExternalPPUDRAMMemoryBlock> external_ppu_dram_memory_config;

		/**
		 * Whether the playbak program has hooks around realtime sections.
		 * If this is the case, a Quiggeldy connection is used and multiple playback programs are
		 * executed, it cannot be guaranteed, that their execution is contiguous.
		 */
		bool has_hooks_around_realtime;

		/**
		 * Hook placed before the initial configuration.
		 */
		stadls::vx::v3::PlaybackProgramBuilder pre_initial_config_hook;

		/**
		 * PPU symbols.
		 */
		std::optional<lola::vx::v3::PPUElfFile::symbols_type> ppu_symbols;

		/**
		 * Playback programs to be executed after intial configuration is established.
		 */
		std::vector<stadls::vx::v3::PlaybackProgram> programs;
	};

	std::map<common::ChipOnConnection, Chip> chips;
};

} // namespace grenade::vx::execution::backend
