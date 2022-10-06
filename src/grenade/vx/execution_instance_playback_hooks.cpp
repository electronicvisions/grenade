#include "grenade/vx/execution_instance_playback_hooks.h"

namespace grenade::vx {

ExecutionInstancePlaybackHooks::ExecutionInstancePlaybackHooks(
    stadls::vx::v3::PlaybackProgramBuilder& pre_static_config,
    stadls::vx::v3::PlaybackProgramBuilder& pre_realtime,
    stadls::vx::v3::PlaybackProgramBuilder& inside_realtime_begin,
    stadls::vx::v3::PlaybackProgramBuilder& inside_realtime_end,
    stadls::vx::v3::PlaybackProgramBuilder& post_realtime) :
    pre_static_config(std::move(pre_static_config)),
    pre_realtime(std::move(pre_realtime)),
    inside_realtime_begin(std::move(inside_realtime_begin)),
    inside_realtime_end(std::move(inside_realtime_end)),
    post_realtime(std::move(post_realtime))
{}

} // namespace grenade::vx
