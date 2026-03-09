#include <gtest/gtest.h>

#include "grenade/vx/execution/backend/initialized_connection.h"
#include "grenade/vx/execution/backend/initialized_connection_run.h"
#include "stadls/vx/v3/playback_program.h"
#include "stadls/vx/v3/playback_program_builder.h"

TEST(InitializedConnection_run, Empty)
{
	grenade::vx::execution::backend::InitializedConnection connection;

	stadls::vx::v3::PlaybackProgramBuilder builder;
	std::vector<stadls::vx::v3::PlaybackProgram> programs{builder.done()};
	std::vector<std::reference_wrapper<stadls::vx::v3::PlaybackProgram>> programs_wrapped;
	programs_wrapped.push_back(programs.at(0));
	grenade::vx::execution::backend::run(connection, programs_wrapped);
	grenade::vx::execution::backend::run(connection, std::move(programs));
}
