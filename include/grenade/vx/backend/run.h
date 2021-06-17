#pragma once
#include "hate/visibility.h"
#include "hxcomm/vx/connection_variant.h"
#include "stadls/vx/run_time_info.h"
#include "stadls/vx/v2/playback_program.h"

namespace grenade::vx::backend {

/**
 * Execute given playback program using the given connection.
 * @param connection Connection to run on
 * @param program Program to execute
 * @return Run time information of execution
 */
stadls::vx::RunTimeInfo run(
    hxcomm::vx::ConnectionVariant& connection,
    stadls::vx::v2::PlaybackProgram& program) SYMBOL_VISIBLE;

/**
 * Execute given playback program using the given connection.
 * @param connection Connection to run on
 * @param program Program to execute
 * @return Run time information of execution
 */
stadls::vx::RunTimeInfo run(
    hxcomm::vx::ConnectionVariant& connection,
    stadls::vx::v2::PlaybackProgram&& program) SYMBOL_VISIBLE;

} // namespace grenade::vx::backend
