#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/vertex/background_spike_source.h"

using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex;

TEST(BackgroundSpikeSource, General)
{
	BackgroundSpikeSource::Config config;
	config.set_enable(!config.get_enable());
	BackgroundSpikeSource::Coordinate coordinate(1);
	BackgroundSpikeSource vertex(config, coordinate);

	EXPECT_FALSE(vertex.variadic_input);
	EXPECT_EQ(vertex.inputs().size(), 0);
	EXPECT_EQ(vertex.output().size, 1);
	EXPECT_EQ(vertex.output().type, ConnectionType::CrossbarInputLabel);
	EXPECT_EQ(vertex.get_config(), config);
	EXPECT_EQ(vertex.get_coordinate(), coordinate);

	BackgroundSpikeSource vertex_copy(vertex);
	EXPECT_EQ(vertex_copy, vertex);
	config.set_enable(!config.get_enable());
	BackgroundSpikeSource vertex1(config, coordinate);
	EXPECT_NE(vertex1, vertex);
	config.set_enable(!config.get_enable());
	BackgroundSpikeSource vertex2(config, BackgroundSpikeSource::Coordinate(2));
	EXPECT_NE(vertex2, vertex);
}
