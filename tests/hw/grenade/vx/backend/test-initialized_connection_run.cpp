#include <gtest/gtest.h>

#include "grenade/vx/execution/backend/initialized_connection.h"
#include "grenade/vx/execution/backend/initialized_connection_run.h"
#include "stadls/vx/v3/playback_program.h"
#include "stadls/vx/v3/playback_program_builder.h"

TEST(InitializedConnection_run, Empty)
{
	grenade::vx::execution::backend::InitializedConnection connection;

	stadls::vx::v3::PlaybackProgramBuilder builder;
	stadls::vx::v3::PlaybackProgram program = builder.done();
	std::vector<stadls::vx::v3::PlaybackProgram> programs;
	for (size_t i = 0; i < connection.size(); i++) {
		programs.push_back(program);
	}
	std::vector<std::reference_wrapper<stadls::vx::v3::PlaybackProgram>> programs_wrapped;
	for (size_t i = 0; i < connection.size(); i++) {
		programs_wrapped.push_back(program);
	}
	grenade::vx::execution::backend::run(connection, programs_wrapped);
	grenade::vx::execution::backend::run(connection, std::move(programs));
}
