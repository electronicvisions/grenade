#include <gtest/gtest.h>

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/backend/run.h"
#include "stadls/vx/v2/playback_program.h"
#include "stadls/vx/v2/playback_program_builder.h"

TEST(run, Empty)
{
	grenade::vx::backend::Connection connection;

	stadls::vx::v2::PlaybackProgramBuilder builder;
	auto program = builder.done();
	grenade::vx::backend::run(connection, program);
	grenade::vx::backend::run(connection, builder.done());
}
