#include <gtest/gtest.h>

#include "grenade/vx/network/projection.h"

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v2;
using namespace halco::common;

TEST(network_Projection_Connection, General)
{
	Projection::Connection connection(12, 13, Projection::Connection::Weight(14));

	Projection::Connection connection_copy(connection);
	EXPECT_EQ(connection, connection_copy);
	connection_copy.index_pre = 11;
	EXPECT_NE(connection, connection_copy);
	connection_copy.index_pre = 12;
	EXPECT_EQ(connection, connection_copy);
	connection_copy.index_post = 12;
	EXPECT_NE(connection, connection_copy);
	connection_copy.index_post = 13;
	EXPECT_EQ(connection, connection_copy);
	connection_copy.weight = Projection::Connection::Weight(13);
	EXPECT_NE(connection, connection_copy);
}

TEST(network_Projection, General)
{
	Projection projection(
	    Projection::ReceptorType::excitatory,
	    {Projection::Connection(12, 13, Projection::Connection::Weight(14))},
	    PopulationDescriptor(15), PopulationDescriptor(16));

	Projection projection_copy(projection);
	EXPECT_EQ(projection, projection_copy);
	projection_copy.receptor_type = Projection::ReceptorType::inhibitory;
	EXPECT_NE(projection, projection_copy);
	projection_copy.receptor_type = Projection::ReceptorType::excitatory;
	EXPECT_EQ(projection, projection_copy);
	projection_copy.connections.at(0) =
	    Projection::Connection(11, 12, Projection::Connection::Weight(13));
	EXPECT_NE(projection, projection_copy);
	projection_copy.connections.at(0) =
	    Projection::Connection(12, 13, Projection::Connection::Weight(14));
	EXPECT_EQ(projection, projection_copy);
	projection_copy.population_pre = PopulationDescriptor(14);
	EXPECT_NE(projection, projection_copy);
	projection_copy.population_pre = PopulationDescriptor(15);
	EXPECT_EQ(projection, projection_copy);
	projection_copy.population_post = PopulationDescriptor(15);
	EXPECT_NE(projection, projection_copy);
}
