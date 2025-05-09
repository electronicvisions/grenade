#include <gtest/gtest.h>

#include "grenade/vx/execution/backend/initialized_connection.h"
#include "grenade/vx/execution/backend/run.h"
#include "stadls/vx/v3/playback_program.h"
#include "stadls/vx/v3/playback_program_builder.h"

TEST(run, Empty)
{
	grenade::vx::execution::backend::InitializedConnection connection;

	stadls::vx::v3::PlaybackProgramBuilder builder;
	auto program = builder.done();
	grenade::vx::execution::backend::run(connection, program);
	grenade::vx::execution::backend::run(connection, builder.done());
}
