#include "grenade/vx/signal_flow/execution_instance_hooks.h"

namespace grenade::vx::signal_flow {

ExecutionInstanceHooks::ExecutionInstanceHooks(
    stadls::vx::v3::PlaybackProgramBuilder& pre_static_config,
    stadls::vx::v3::PlaybackProgramBuilder& pre_realtime,
    stadls::vx::v3::PlaybackProgramBuilder& inside_realtime_begin,
    stadls::vx::v3::AbsoluteTimePlaybackProgramBuilder& inside_realtime,
    stadls::vx::v3::PlaybackProgramBuilder& inside_realtime_end,
    stadls::vx::v3::PlaybackProgramBuilder& post_realtime,
    WritePPUSymbols const& write_ppu_symbols,
    ReadPPUSymbols const& read_ppu_symbols) :
    pre_static_config(std::move(pre_static_config)),
    pre_realtime(std::move(pre_realtime)),
    inside_realtime_begin(std::move(inside_realtime_begin)),
    inside_realtime(std::move(inside_realtime)),
    inside_realtime_end(std::move(inside_realtime_end)),
    post_realtime(std::move(post_realtime)),
    write_ppu_symbols(write_ppu_symbols),
    read_ppu_symbols(read_ppu_symbols)
{}

} // namespace grenade::vx::signal_flow
