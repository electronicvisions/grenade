#pragma once
#include "grenade/vx/execution/backend/run_time_info.h"
#include "hate/visibility.h"

namespace grenade::vx::execution::backend {

struct StatefulConnection;
struct PlaybackProgram;

/**
 * Execute given playback program using the given connection.
 * @param connection StatefulConnection to run on
 * @param program Program to execute
 */
RunTimeInfo run(StatefulConnection& connection, PlaybackProgram& program) SYMBOL_VISIBLE;

/**
 * Execute given playback program using the given connection.
 * @param connection StatefulConnection to run on
 * @param program Program to execute
 */
RunTimeInfo run(StatefulConnection& connection, PlaybackProgram&& program) SYMBOL_VISIBLE;

} // namespace grenade::vx::execution::backend
