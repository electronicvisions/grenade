#pragma once
#include "hate/visibility.h"
#include "stadls/vx/run_time_info.h"

namespace stadls::vx {
struct PlaybackProgram;

namespace v3 {
using PlaybackProgram = stadls::vx::PlaybackProgram;
} // namespace v3

} // namespace stadls::vx

namespace grenade::vx::execution::backend {

struct InitializedConnection;

/**
 * Execute given playback program using the given connection.
 * @param connection InitializedConnection to run on
 * @param programs Programs to execute
 * @return Run time information of execution
 */
stadls::vx::RunTimeInfo run(
    InitializedConnection& connection,
    std::vector<std::reference_wrapper<stadls::vx::v3::PlaybackProgram>> const& programs)
    SYMBOL_VISIBLE;

/**
 * Execute given playback program using the given connection.
 * @param connection InitializedConnection to run on
 * @param programs Programs to execute
 * @return Run time information of execution
 */
stadls::vx::RunTimeInfo run(
    InitializedConnection& connection,
    std::vector<stadls::vx::v3::PlaybackProgram>&& programs) SYMBOL_VISIBLE;

} // namespace grenade::vx::execution::backend
