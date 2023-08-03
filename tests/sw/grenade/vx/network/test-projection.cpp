#include <gtest/gtest.h>

#include "grenade/vx/network/projection.h"

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(network_Projection_Connection, General)
{
	Projection::Connection connection(
	    {12, CompartmentOnLogicalNeuron()}, {13, CompartmentOnLogicalNeuron()},
	    Projection::Connection::Weight(14));

	Projection::Connection connection_copy(connection);
	EXPECT_EQ(connection, connection_copy);
	connection_copy.index_pre = {11, CompartmentOnLogicalNeuron()};
	EXPECT_NE(connection, connection_copy);
	connection_copy.index_pre = {12, CompartmentOnLogicalNeuron()};
	EXPECT_EQ(connection, connection_copy);
	connection_copy.index_post = {12, CompartmentOnLogicalNeuron()};
	EXPECT_NE(connection, connection_copy);
	connection_copy.index_post = {13, CompartmentOnLogicalNeuron()};
	EXPECT_EQ(connection, connection_copy);
	connection_copy.weight = Projection::Connection::Weight(13);
	EXPECT_NE(connection, connection_copy);
}

TEST(network_Projection, General)
{
	Projection projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory),
	    {Projection::Connection(
	        {12, CompartmentOnLogicalNeuron()}, {13, CompartmentOnLogicalNeuron()},
	        Projection::Connection::Weight(14))},
	    PopulationOnExecutionInstance(15), PopulationOnExecutionInstance(16));

	Projection projection_copy(projection);
	EXPECT_EQ(projection, projection_copy);
	projection_copy.receptor = Receptor(Receptor::ID(), Receptor::Type::inhibitory);
	EXPECT_NE(projection, projection_copy);
	projection_copy.receptor = Receptor(Receptor::ID(), Receptor::Type::excitatory);
	EXPECT_EQ(projection, projection_copy);
	projection_copy.connections.at(0) = Projection::Connection(
	    {11, CompartmentOnLogicalNeuron()}, {12, CompartmentOnLogicalNeuron()},
	    Projection::Connection::Weight(13));
	EXPECT_NE(projection, projection_copy);
	projection_copy.connections.at(0) = Projection::Connection(
	    {12, CompartmentOnLogicalNeuron()}, {13, CompartmentOnLogicalNeuron()},
	    Projection::Connection::Weight(14));
	EXPECT_EQ(projection, projection_copy);
	projection_copy.population_pre = PopulationOnExecutionInstance(14);
	EXPECT_NE(projection, projection_copy);
	projection_copy.population_pre = PopulationOnExecutionInstance(15);
	EXPECT_EQ(projection, projection_copy);
	projection_copy.population_post = PopulationOnExecutionInstance(15);
	EXPECT_NE(projection, projection_copy);
}
