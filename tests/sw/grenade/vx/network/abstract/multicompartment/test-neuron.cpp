#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/capacitance.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_conductance.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_current.h"
#include "grenade/vx/network/abstract/multicompartment/neuron.h"
#include "grenade/vx/network/abstract/multicompartment/resource_manager.h"
#include "grenade/vx/network/abstract/multicompartment/synaptic_input_environment.h"
#include <gtest/gtest.h>

using namespace grenade::vx::network::abstract;

TEST(multicompartment_neuron, General)
{
	// Neuron
	Neuron neuron;

	// Compartments
	Compartment compartment_a;
	Compartment compartment_b;
	Compartment compartment_c;

	// Mechanisms
	MechanismCapacitance membrane;
	MechanismSynapticInputCurrent synaptic_current;
	MechanismSynapticInputConductance synaptic_conductance;

	// Valid parameter spaces
	EXPECT_NO_THROW(MechanismCapacitance::ParameterSpace(ParameterInterval<double>(1, 10)));
	EXPECT_NO_THROW(MechanismCapacitance::ParameterSpace(ParameterInterval<double>(5, 20)));
	EXPECT_NO_THROW(MechanismSynapticInputCurrent::ParameterSpace(
	    ParameterInterval<double>(1, 1), ParameterInterval<double>(2, 2)));
	EXPECT_NO_THROW(MechanismSynapticInputCurrent::ParameterSpace(
	    ParameterInterval<double>(1, 1), ParameterInterval<double>(2, 2)));
	EXPECT_NO_THROW(MechanismSynapticInputConductance::ParameterSpace(
	    ParameterInterval<double>(1, 1), ParameterInterval<double>(1, 1),
	    ParameterInterval<double>(2, 2)));

	// Invalid parameter spaces
	EXPECT_THROW(
	    MechanismCapacitance::ParameterSpace(ParameterInterval<double>(7, 3)),
	    std::invalid_argument);
	EXPECT_THROW(
	    MechanismSynapticInputCurrent::ParameterSpace(
	        ParameterInterval<double>(1, 3), ParameterInterval<double>(4, 1)),
	    std::invalid_argument);

	// Add Mechanisms to Compartments
	[[maybe_unused]] auto const membrane_on_compartment_a = compartment_a.add(membrane);
	[[maybe_unused]] auto const membrane_on_compartment_b = compartment_b.add(membrane);
	[[maybe_unused]] auto const membrane_on_compartment_c = compartment_c.add(membrane);
	[[maybe_unused]] auto const synaptic_current_a_on_compartment_a =
	    compartment_a.add(synaptic_current);
	[[maybe_unused]] auto const synaptic_conductance_on_compartment_a =
	    compartment_a.add(synaptic_conductance);
	[[maybe_unused]] auto const synaptic_current_b_on_compartment_b =
	    compartment_b.add(synaptic_current);

	// Not two mechanisms of same type on one compartment
	EXPECT_THROW(compartment_a.add(membrane), std::invalid_argument);

	compartment_c.set(membrane_on_compartment_c, MechanismCapacitance());
	MechanismCapacitance::ParameterSpace(ParameterInterval<double>(20, 20));

	EXPECT_FALSE(synaptic_conductance == synaptic_current);
	EXPECT_TRUE(synaptic_current == synaptic_current);

	compartment_a.remove(synaptic_conductance_on_compartment_a);
	compartment_a.remove(synaptic_current_a_on_compartment_a);
	compartment_b.remove(synaptic_current_b_on_compartment_b);
	compartment_b.remove(membrane_on_compartment_b);

	EXPECT_THROW(compartment_b.remove(membrane_on_compartment_b), std::invalid_argument);
	EXPECT_THROW(compartment_b.get(membrane_on_compartment_b), std::invalid_argument);

	// Add Compartments to Neuron
	auto const compartment_a_on_neuron = neuron.add_compartment(compartment_a);
	auto const compartment_b_on_neuron = neuron.add_compartment(compartment_b);
	auto const compartment_c_on_neuron = neuron.add_compartment(compartment_c);

	EXPECT_EQ(neuron.num_compartments(), 3);
	EXPECT_EQ(neuron.get(compartment_a_on_neuron), compartment_a);
	EXPECT_EQ(neuron.get(compartment_a_on_neuron).get(membrane_on_compartment_a), membrane);

	// Add Compartment-Connections to Neuron
	CompartmentConnectionConductance connection_conductance_1;
	CompartmentConnectionConductance connection_conductance_2;
	CompartmentConnectionConductance connection_conductance_3;
	connection_conductance_3 = CompartmentConnectionConductance(10);

	EXPECT_EQ(connection_conductance_2, connection_conductance_2);
	EXPECT_NE(connection_conductance_1, connection_conductance_3);

	[[maybe_unused]] auto const compartment_connection_ab_on_neuron =
	    neuron.add_compartment_connection(
	        compartment_a_on_neuron, compartment_b_on_neuron, connection_conductance_1);
	[[maybe_unused]] auto const compartment_connection_ac_on_neuron =
	    neuron.add_compartment_connection(
	        compartment_a_on_neuron, compartment_c_on_neuron, connection_conductance_2);

	EXPECT_EQ(neuron.num_compartment_connections(), 2);
	EXPECT_EQ(neuron.get_compartment_degree(compartment_a_on_neuron), 2);
	EXPECT_EQ(neuron.get_compartment_degree(compartment_b_on_neuron), 1);
	EXPECT_NE(compartment_connection_ab_on_neuron, compartment_connection_ac_on_neuron);

	neuron.remove_compartment_connection(compartment_connection_ac_on_neuron);

	// Source and Target of CompartmentConnections
	EXPECT_EQ(neuron.source(compartment_connection_ab_on_neuron), compartment_a_on_neuron);
	EXPECT_EQ(neuron.target(compartment_connection_ab_on_neuron), compartment_b_on_neuron);

	// Contains
	EXPECT_TRUE(neuron.contains(compartment_a_on_neuron));
	neuron.remove_compartment(compartment_c_on_neuron);
	EXPECT_FALSE(neuron.contains(compartment_c_on_neuron));
}