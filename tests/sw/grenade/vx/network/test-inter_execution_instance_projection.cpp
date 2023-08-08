#include <gtest/gtest.h>

#include "grenade/vx/network/inter_execution_instance_projection.h"

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(network_InterExecutionInstanceProjection_Connection, General)
{
	InterExecutionInstanceProjection::Connection connection(
	    {12, CompartmentOnLogicalNeuron()}, {13, CompartmentOnLogicalNeuron()});

	InterExecutionInstanceProjection::Connection connection_copy(connection);
	EXPECT_EQ(connection, connection_copy);
	connection_copy.index_pre = {11, CompartmentOnLogicalNeuron()};
	EXPECT_NE(connection, connection_copy);
	connection_copy.index_pre = {12, CompartmentOnLogicalNeuron()};
	EXPECT_EQ(connection, connection_copy);
	connection_copy.index_post = {12, CompartmentOnLogicalNeuron()};
	EXPECT_NE(connection, connection_copy);
	connection_copy.index_post = {13, CompartmentOnLogicalNeuron()};
	EXPECT_EQ(connection, connection_copy);
}

TEST(network_InterExecutionInstanceProjection, General)
{
	InterExecutionInstanceProjection projection(
	    {InterExecutionInstanceProjection::Connection(
	        {12, CompartmentOnLogicalNeuron()}, {13, CompartmentOnLogicalNeuron()})},
	    PopulationOnNetwork(PopulationOnExecutionInstance(15)),
	    PopulationOnNetwork(PopulationOnExecutionInstance(16)));

	InterExecutionInstanceProjection projection_copy(projection);
	EXPECT_EQ(projection, projection_copy);
	projection_copy.connections.at(0) = InterExecutionInstanceProjection::Connection(
	    {11, CompartmentOnLogicalNeuron()}, {12, CompartmentOnLogicalNeuron()});
	EXPECT_NE(projection, projection_copy);
	projection_copy.connections.at(0) = InterExecutionInstanceProjection::Connection(
	    {12, CompartmentOnLogicalNeuron()}, {13, CompartmentOnLogicalNeuron()});
	EXPECT_EQ(projection, projection_copy);
	projection_copy.population_pre = PopulationOnNetwork(PopulationOnExecutionInstance(14));
	EXPECT_NE(projection, projection_copy);
	projection_copy.population_pre = PopulationOnNetwork(PopulationOnExecutionInstance(15));
	EXPECT_EQ(projection, projection_copy);
	projection_copy.population_post = PopulationOnNetwork(PopulationOnExecutionInstance(15));
	EXPECT_NE(projection, projection_copy);
}
