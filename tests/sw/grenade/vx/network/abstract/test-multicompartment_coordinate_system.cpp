#include "grenade/vx/network/abstract/multicompartment_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_placement_coordinate_system.h"
#include <gtest/gtest.h>

#include <iostream>

using namespace grenade::vx::network::abstract;


TEST(MulticompartmentPlacementCoordinates, BaseTest)
{
	CoordinateSystem coordinates;

	/*
	     _ _ _
	    | | $ $
	0-0-0 0 1 2
	|
	0-0 x x x x
	*/

	coordinates[0][0].switch_right = 1;
	coordinates[0][0].switch_top_bottom = 1;

	coordinates[1][0].switch_right = 1;
	coordinates[1][0].switch_top_bottom = 1;

	coordinates[0][1].switch_right = 1;

	coordinates[1][1].switch_right = 1;

	coordinates[0][2].switch_circuit_shared = 1;
	coordinates[0][2].switch_shared_right = 1;

	coordinates[0][3].switch_circuit_shared = 1;
	coordinates[0][3].switch_shared_right = 1;

	coordinates[0][4].switch_circuit_shared_conductance = 1;
	coordinates[0][4].switch_shared_right = 1;

	coordinates[0][5].switch_circuit_shared_conductance = 1;

	// Testing connection detection
	EXPECT_TRUE(coordinates.connected(0, 0));
	EXPECT_TRUE(coordinates.connected(0, 1));
	EXPECT_TRUE(coordinates.connected(1, 0));
	EXPECT_TRUE(coordinates.connected(1, 1));
	EXPECT_TRUE(coordinates.connected(4, 0));
	EXPECT_TRUE(coordinates.connected(5, 0));

	// Testing direct connections
	EXPECT_TRUE(coordinates.connected_right(0, 0));
	EXPECT_FALSE(coordinates.connected_top_bottom(1, 1));
	EXPECT_TRUE(coordinates.connected_left(1, 0));

	// Testing direct connection over shared line
	EXPECT_EQ(coordinates.connected_shared_short(2, 0).size(), 1);
	EXPECT_EQ(coordinates.connected_shared_short(2, 0)[0].first, 3);
	EXPECT_EQ(coordinates.connected_shared_short(2, 0)[0].second, 0);

	// Testing for invalid connections
	EXPECT_FALSE(coordinates.has_empty_connections(255));
	EXPECT_FALSE(coordinates.has_double_connections(255));
	EXPECT_FALSE(coordinates.has_empty_connections(255));
	EXPECT_FALSE(coordinates.has_double_connections(255));
	EXPECT_FALSE(coordinates.short_circuit(255));

	// Test neuron
	Neuron neuron;
	Compartment compartment_a;
	CompartmentOnNeuron compartment_a_on_neuron = neuron.add_compartment(compartment_a);

	// Assigning compartment to connected circuits
	coordinates.assign_compartment_adjacent(0, 0, compartment_a_on_neuron);
	EXPECT_TRUE(coordinates.get_compartment(0, 0));
	EXPECT_EQ(coordinates.get_compartment(0, 0).value(), compartment_a_on_neuron);
	EXPECT_TRUE(coordinates.get_compartment(0, 1));
	EXPECT_EQ(coordinates.get_compartment(0, 1).value(), compartment_a_on_neuron);
	EXPECT_TRUE(coordinates.get_compartment(1, 0));
	EXPECT_EQ(coordinates.get_compartment(1, 0).value(), compartment_a_on_neuron);
	EXPECT_TRUE(coordinates.get_compartment(1, 1));
	EXPECT_EQ(coordinates.get_compartment(1, 1).value(), compartment_a_on_neuron);
	EXPECT_TRUE(coordinates.get_compartment(2, 0));
	EXPECT_EQ(coordinates.get_compartment(2, 0).value(), compartment_a_on_neuron);

	// Detect and clear empty connection
	coordinates[0][10].switch_circuit_shared = 1;
	EXPECT_TRUE(coordinates.has_empty_connections(255));
	coordinates.clear_empty_connections(255);
	EXPECT_FALSE(coordinates.has_empty_connections(255));
	EXPECT_FALSE(coordinates.has_empty_connections(255));

	// New coordinate system to test connection to not adjacent neuron circuits
	coordinates.clear();

	coordinates[0][0].switch_circuit_shared = 1;
	coordinates[0][0].switch_shared_right = 1;

	coordinates[0][1].switch_shared_right = 1;

	coordinates[0][2].switch_circuit_shared_conductance = 1;

	/*
	 _ _
	|   &
	0 x 1 x x x x
	x x x x x x x
	*/

	EXPECT_FALSE(coordinates.has_empty_connections(255));
	EXPECT_FALSE(coordinates.has_empty_connections(255));
	EXPECT_EQ(coordinates.connected_shared_short(0, 0).size(), 0);
	EXPECT_EQ(coordinates.connected_shared_conductance(0, 0).size(), 1);
	EXPECT_EQ(coordinates.connected_shared_conductance(0, 0).at(0).first, 2);
	EXPECT_EQ(coordinates.connected_shared_conductance(0, 0).at(0).second, 0);

	/* Check for connections via conductances to multiple different compartments.
	 _ _ _ _
	|   &   &
	0 x 1 x 2 x x
	x x x x x x x
	*/
	coordinates[0][2].switch_shared_right = 1;

	coordinates[0][3].switch_shared_right = 1;

	coordinates[0][4].switch_circuit_shared_conductance = 1;

	EXPECT_EQ(coordinates.connected_shared_short(0, 0).size(), 0);
	EXPECT_EQ(coordinates.connected_shared_conductance(0, 0).size(), 2);
	EXPECT_EQ(coordinates.connected_shared_conductance(0, 0).at(0).first, 2);
	EXPECT_EQ(coordinates.connected_shared_conductance(0, 0).at(0).second, 0);
	EXPECT_EQ(coordinates.connected_shared_conductance(0, 0).at(1).first, 4);
	EXPECT_EQ(coordinates.connected_shared_conductance(0, 0).at(1).second, 0);

	/* Check for connections via conductance to multiple different compartments combined with a
	short circuit connection to itself.
	 _ _ _ _
	| | &   &
	0 0 1 x 2 x x
	x x x x x x x
	*/
	coordinates[0][1].switch_circuit_shared = 1;
	EXPECT_EQ(coordinates.connected_shared_short(0, 0).size(), 1);
	EXPECT_EQ(coordinates.connected_shared_conductance(0, 0).size(), 2);

	coordinates.assign_compartment_adjacent(0, 0, compartment_a_on_neuron);
	EXPECT_TRUE(coordinates.get_compartment(0, 0));
	EXPECT_EQ(coordinates.get_compartment(0, 0).value(), compartment_a_on_neuron);
	EXPECT_FALSE(coordinates.get_compartment(2, 0));
}


TEST(MulticompartmentPlacementCoordiantes, EmptyConnections)
{
	CoordinateSystem coordinates;

	/* Detection of empty connection. Checking case where only shared line is closed but
	not connected to any neuron cirucit.
	 _

	0 x x x x x x
	x x x x x x x
	*/
	coordinates[0][0].switch_shared_right = 1;
	EXPECT_TRUE(coordinates.has_empty_connections(255));

	/* Detection of empty connection. Checking case where only shared line is closed but
	only connected to one neuron circuit and having an open end.
	 _
	|
	0 x x x x x x
	x x x x x x x
	*/
	coordinates[0][0].switch_circuit_shared = 1;
	EXPECT_TRUE(coordinates.has_empty_connections(255));

	/* Detection of empty connections. Checking case where no empty connection exists.
	 _
	| &
	0 1 x x x x
	x x x x x x x
	*/
	coordinates[0][1].switch_circuit_shared_conductance = 1;
	EXPECT_FALSE(coordinates.has_empty_connections(255));

	/* Clearing of empty connections.
	 _ _
	| &
	0 1 x x x x x
	x x x x x x x
	*/
	coordinates[0][1].switch_shared_right = 1;
	coordinates.clear_empty_connections(255);
	EXPECT_FALSE(coordinates.has_empty_connections(255));
	EXPECT_EQ(coordinates[0][0].switch_circuit_shared, 1);
	EXPECT_EQ(coordinates[0][0].switch_shared_right, 1);
	EXPECT_EQ(coordinates[0][1].switch_shared_right, 0);
	EXPECT_EQ(coordinates[0][1].switch_circuit_shared_conductance, 1);


	/*
	 _ _ _
	| &
	0 1 x x x x x
	x x x x x x x
	*/
	coordinates[0][1].switch_shared_right = 1;
	coordinates[0][2].switch_shared_right = 1;
	coordinates.clear_empty_connections(255);
	EXPECT_FALSE(coordinates.has_empty_connections(255));
	EXPECT_EQ(coordinates[0][1].switch_shared_right, 0);
	EXPECT_EQ(coordinates[0][2].switch_shared_right, 0);
	EXPECT_EQ(coordinates[0][1].switch_circuit_shared_conductance, 1);

	/* Clearing of empty connections in the presence of another valid connection.
	 _     _
	| &   |
	0 1 x x x x x
	x x x x x x x
	*/
	coordinates[0][3].switch_circuit_shared = 1;
	coordinates[0][3].switch_shared_right = 1;
	coordinates.clear_empty_connections(255);
	EXPECT_FALSE(coordinates.has_empty_connections(255));
	EXPECT_EQ(coordinates[0][3].switch_circuit_shared, 0);
	EXPECT_EQ(coordinates[0][3].switch_shared_right, 0);
	EXPECT_EQ(coordinates[0][1].switch_circuit_shared_conductance, 1);

	/* Clearing of empty connections should do nothing.
	 _ _ _
	| & & &
	0 1 2 3 x x x
	x x x x x x x
	*/
	coordinates.clear();
	coordinates[0][0].switch_shared_right = 1;
	coordinates[0][1].switch_shared_right = 1;
	coordinates[0][2].switch_shared_right = 1;
	coordinates[0][0].switch_circuit_shared = 1;
	coordinates[0][1].switch_circuit_shared_conductance = 1;
	coordinates[0][2].switch_circuit_shared_conductance = 1;
	coordinates[0][3].switch_circuit_shared_conductance = 1;


	coordinates.clear_empty_connections(255);
	EXPECT_FALSE(coordinates.has_empty_connections(255));
	EXPECT_EQ(coordinates[0][0].switch_shared_right, 1);
	EXPECT_EQ(coordinates[0][1].switch_shared_right, 1);
	EXPECT_EQ(coordinates[0][2].switch_shared_right, 1);
	EXPECT_EQ(coordinates[0][0].switch_circuit_shared, 1);
	EXPECT_EQ(coordinates[0][1].switch_circuit_shared_conductance, 1);
	EXPECT_EQ(coordinates[0][2].switch_circuit_shared_conductance, 1);
	EXPECT_EQ(coordinates[0][3].switch_circuit_shared_conductance, 1);


	/* Clearing empty connections in combination with valid connection at left limit of
	coordinate system.
	 _ _ _
	  & | &
	x 1 2 3 x x x
	x x x x x x x
	*/
	coordinates.clear();
	coordinates[0][0].switch_shared_right = 1;
	coordinates[0][1].switch_shared_right = 1;
	coordinates[0][2].switch_shared_right = 1;
	coordinates[0][1].switch_circuit_shared_conductance = 1;
	coordinates[0][2].switch_circuit_shared = 1;
	coordinates[0][3].switch_circuit_shared_conductance = 1;


	coordinates.clear_empty_connections(255);
	EXPECT_FALSE(coordinates.has_empty_connections(255));
	EXPECT_EQ(coordinates[0][0].switch_shared_right, 0);
	EXPECT_EQ(coordinates[0][1].switch_shared_right, 1);
	EXPECT_EQ(coordinates[0][2].switch_shared_right, 1);
	EXPECT_EQ(coordinates[0][1].switch_circuit_shared_conductance, 1);
	EXPECT_EQ(coordinates[0][2].switch_circuit_shared, 1);
	EXPECT_EQ(coordinates[0][3].switch_circuit_shared_conductance, 1);
}
