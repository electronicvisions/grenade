#include <gtest/gtest.h>

#include "grenade/vx/execution/backend/initialized_connection.h"
#include "hxcomm/vx/connection_from_env.h"
#include "stadls/vx/v3/init_generator.h"

TEST(InitializedConnection, General)
{
	std::string hxcomm_unique_identifier;
	{
		auto hxcomm_connection = hxcomm::vx::get_connection_from_env();
		hxcomm_unique_identifier =
		    std::visit([](auto const& c) { return c.get_unique_identifier(); }, hxcomm_connection);

		grenade::vx::execution::backend::InitializedConnection connection(
		    std::move(hxcomm_connection));
		EXPECT_EQ(connection.get_unique_identifier(std::nullopt), hxcomm_unique_identifier);

		auto released_hxcomm_connection = std::move(connection.release());
		auto const released_hxcomm_unique_identifier = std::visit(
		    [](auto const& c) { return c.get_unique_identifier(); }, released_hxcomm_connection);
		EXPECT_EQ(released_hxcomm_unique_identifier, hxcomm_unique_identifier);
	}

	{
		grenade::vx::execution::backend::InitializedConnection connection;
		EXPECT_EQ(connection.get_unique_identifier(std::nullopt), hxcomm_unique_identifier);

		auto const time_info_first = connection.get_time_info();
		auto const time_info_second = connection.get_time_info();
		EXPECT_GE(time_info_second.encode_duration, time_info_first.encode_duration);
		EXPECT_GE(time_info_second.decode_duration, time_info_first.decode_duration);
		EXPECT_GE(time_info_second.commit_duration, time_info_first.commit_duration);
		EXPECT_GE(time_info_second.execution_duration, time_info_first.execution_duration);
	}
	{
		auto hxcomm_connection = hxcomm::vx::get_connection_from_env();
		grenade::vx::execution::backend::InitializedConnection connection(
		    std::move(hxcomm_connection),
		    stadls::vx::v3::DigitalInit(std::visit(
		        [](auto const& connection) { return connection.get_hwdb_entry(); },
		        hxcomm_connection)));
		EXPECT_EQ(connection.get_unique_identifier(std::nullopt), hxcomm_unique_identifier);
	}
}
