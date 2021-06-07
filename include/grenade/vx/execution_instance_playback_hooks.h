#pragma once
#include "grenade/vx/genpybind.h"
#include "stadls/vx/v2/playback_program_builder.h"

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

/**
 * Playback program hooks for an execution instance.
 */
struct GENPYBIND(visible) ExecutionInstancePlaybackHooks
{
	ExecutionInstancePlaybackHooks() = default;
	ExecutionInstancePlaybackHooks(
	    stadls::vx::v2::PlaybackProgramBuilder& pre_static_config,
	    stadls::vx::v2::PlaybackProgramBuilder& pre_realtime,
	    stadls::vx::v2::PlaybackProgramBuilder& post_realtime) SYMBOL_VISIBLE;
	ExecutionInstancePlaybackHooks(ExecutionInstancePlaybackHooks const&) = delete;
	ExecutionInstancePlaybackHooks(ExecutionInstancePlaybackHooks&&) = default;
	ExecutionInstancePlaybackHooks& operator=(ExecutionInstancePlaybackHooks const&) = delete;
	ExecutionInstancePlaybackHooks& operator=(ExecutionInstancePlaybackHooks&&) = default;

	stadls::vx::v2::PlaybackProgramBuilder GENPYBIND(hidden) pre_static_config;
	stadls::vx::v2::PlaybackProgramBuilder GENPYBIND(hidden) pre_realtime;
	stadls::vx::v2::PlaybackProgramBuilder GENPYBIND(hidden) post_realtime;
};

} // namespace grenade::vx
