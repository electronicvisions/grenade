#include "grenade/common/edge.h"

#include "grenade/common/multi_index_sequence/list.h"
#include <stdexcept>
#include <cereal/archives/json.hpp>
#include <gtest/gtest.h>


using namespace grenade::common;

TEST(Edge, General)
{
	ListMultiIndexSequence channels_0({MultiIndex({1}), MultiIndex({2})});
	ListMultiIndexSequence channels_1({MultiIndex({2}), MultiIndex({2})});
	ListMultiIndexSequence channels_2({MultiIndex({2})});

	Edge edge_0(channels_0, channels_1);
	EXPECT_EQ(edge_0.get_channels_on_source(), channels_0);
	EXPECT_EQ(edge_0.get_channels_on_target(), channels_1);
	EXPECT_EQ(edge_0.port_on_source, 0);
	EXPECT_EQ(edge_0.port_on_target, 0);
	EXPECT_EQ((Edge(std::move(*channels_0.copy()), std::move(*channels_1.copy()))), edge_0);

	// not bijective
	EXPECT_THROW(edge_0.set_channels_on_source(channels_2), std::invalid_argument);
	EXPECT_THROW(edge_0.set_channels_on_target(channels_2), std::invalid_argument);
	EXPECT_THROW((Edge(channels_0, channels_2)), std::invalid_argument);
	EXPECT_THROW(
	    (Edge(std::move(*channels_0.copy()), std::move(*channels_2.copy()))),
	    std::invalid_argument);
	EXPECT_THROW((Edge(channels_2, channels_0)), std::invalid_argument);
	EXPECT_THROW((Edge(channels_0, channels_2, 0, 1)), std::invalid_argument);
	EXPECT_THROW(
	    (Edge(std::move(*channels_0.copy()), std::move(*channels_2.copy()), 0, 1)),
	    std::invalid_argument);

	EXPECT_NO_THROW(edge_0.set_channels_on_source(channels_1));
	EXPECT_NO_THROW(edge_0.set_channels_on_target(channels_0));
	EXPECT_EQ(edge_0.get_channels_on_source(), channels_1);
	EXPECT_EQ(edge_0.get_channels_on_target(), channels_0);

	auto edge_0_copy = edge_0.copy();
	assert(edge_0_copy);
	EXPECT_EQ(*edge_0_copy, edge_0);
	auto edge_0_move = edge_0_copy->move();
	assert(edge_0_move);
	EXPECT_EQ(*edge_0_move, edge_0);
	edge_0_move->set_channels_on_source(channels_0);
	EXPECT_NE(*edge_0_move, edge_0);
	edge_0_move->set_channels_on_source(channels_1);
	edge_0_move->set_channels_on_target(channels_1);
	EXPECT_NE(*edge_0_move, edge_0);
	edge_0_move->set_channels_on_target(channels_0);
	edge_0_move->port_on_source = 1;
	EXPECT_NE(*edge_0_move, edge_0);
	edge_0_move->port_on_source = 0;
	edge_0_move->port_on_target = 1;
	EXPECT_NE(*edge_0_move, edge_0);

	Edge edge_1(channels_0, channels_1, 1, 2);
	EXPECT_EQ(edge_1.get_channels_on_source(), channels_0);
	EXPECT_EQ(edge_1.get_channels_on_target(), channels_1);
	EXPECT_EQ(edge_1.port_on_source, 1);
	EXPECT_EQ(edge_1.port_on_target, 2);

	Edge edge_2(std::move(*channels_0.copy()), std::move(*channels_1.copy()), 2, 3);
	EXPECT_EQ(edge_2.get_channels_on_source(), channels_0);
	EXPECT_EQ(edge_2.get_channels_on_target(), channels_1);
	EXPECT_EQ(edge_2.port_on_source, 2);
	EXPECT_EQ(edge_2.port_on_target, 3);
}

TEST(Edge, Cerealization)
{
	ListMultiIndexSequence channels_0({MultiIndex({1}), MultiIndex({2})});
	ListMultiIndexSequence channels_1({MultiIndex({2}), MultiIndex({2})});

	Edge obj1(channels_0, channels_1, 1, 2);

	Edge obj2;

	std::ostringstream ostream;
	{
		cereal::JSONOutputArchive oa(ostream);
		oa(obj1);
	}

	std::istringstream istream(ostream.str());
	{
		cereal::JSONInputArchive ia(istream);
		ia(obj2);
	}

	ASSERT_EQ(obj2, obj1);
}
