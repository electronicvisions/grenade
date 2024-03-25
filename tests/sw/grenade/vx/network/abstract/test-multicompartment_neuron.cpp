#include "grenade/vx/network/abstract/multicompartment_environment.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism/capacitance.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism/synaptic_conductance.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism/synaptic_current.h"
#include "grenade/vx/network/abstract/multicompartment_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_resource_manager.h"
#include "grenade/vx/network/abstract/multicompartment_synaptic_input_environment.h"
#include <gtest/gtest.h>

using namespace grenade::vx::network;

TEST(multicompartment_neuron, General)
{
	// Neuron
	Neuron neuron;

	// Compartments
	Compartment compartment_a;
	Compartment compartment_b;
	Compartment compartment_c;

	// Valid Mechanisms
	MechanismCapacitance membrane_a(MechanismCapacitance::ParameterSpace(
	    ParameterInterval<double>(1, 10),
	    MechanismCapacitance::ParameterSpace::Parameterization(5)));
	MechanismCapacitance membrane_b(MechanismCapacitance::ParameterSpace(
	    ParameterInterval<double>(5, 20),
	    MechanismCapacitance::ParameterSpace::Parameterization(7)));
	MechanismSynapticInputCurrent synaptic_current_a(MechanismSynapticInputCurrent::ParameterSpace(
	    ParameterInterval<double>(1, 1), ParameterInterval<double>(2, 2),
	    MechanismSynapticInputCurrent::ParameterSpace::Parameterization(1, 2)));
	MechanismSynapticInputCurrent synaptic_current_b(MechanismSynapticInputCurrent::ParameterSpace(
	    ParameterInterval<double>(1, 1), ParameterInterval<double>(2, 2),
	    MechanismSynapticInputCurrent::ParameterSpace::Parameterization(1, 2)));
	MechanismSynapticInputConductance synaptic_conductance_a(
	    MechanismSynapticInputConductance::ParameterSpace(
	        ParameterInterval<double>(1, 1), ParameterInterval<double>(1, 1),
	        ParameterInterval<double>(2, 2),
	        MechanismSynapticInputConductance::ParameterSpace::Parameterization(1, 1, 2)));


	// Invalid Mechanisms
	EXPECT_THROW(
	    MechanismCapacitance(MechanismCapacitance::ParameterSpace(
	        ParameterInterval<double>(4, 6),
	        MechanismCapacitance::ParameterSpace::Parameterization(2))),
	    std::invalid_argument);
	EXPECT_THROW(
	    MechanismCapacitance(MechanismCapacitance::ParameterSpace(
	        ParameterInterval<double>(7, 3),
	        MechanismCapacitance::ParameterSpace::Parameterization(4))),
	    std::invalid_argument);
	EXPECT_THROW(
	    MechanismSynapticInputCurrent(MechanismSynapticInputCurrent::ParameterSpace(
	        ParameterInterval<double>(1, 3), ParameterInterval<double>(4, 1),
	        MechanismSynapticInputCurrent::ParameterSpace::Parameterization(2, 2))),
	    std::invalid_argument);
	EXPECT_THROW(
	    MechanismSynapticInputConductance(MechanismSynapticInputConductance::ParameterSpace(
	        ParameterInterval<double>(1, 3), ParameterInterval<double>(1, 1),
	        ParameterInterval<double>(2, 2),
	        MechanismSynapticInputConductance::ParameterSpace::Parameterization(5, 1, 2))),
	    std::invalid_argument);

	// Add Mechanisms to Compartments
	[[maybe_unused]] auto const membrane_on_compartment_a = compartment_a.add(membrane_a);
	[[maybe_unused]] auto const membrane_on_compartment_b = compartment_b.add(membrane_b);
	[[maybe_unused]] auto const membrane_on_compartment_c = compartment_c.add(membrane_b);
	[[maybe_unused]] auto const synaptic_current_a_on_compartment_a =
	    compartment_a.add(synaptic_current_a);
	[[maybe_unused]] auto const synaptic_conductance_on_compartment_a =
	    compartment_a.add(synaptic_conductance_a);
	[[maybe_unused]] auto const synaptic_current_b_on_compartment_b =
	    compartment_b.add(synaptic_current_b);

	EXPECT_THROW(compartment_a.add(membrane_b), std::invalid_argument);
	EXPECT_THROW(compartment_a.add(membrane_a), std::invalid_argument);

	compartment_c.set(
	    membrane_on_compartment_c,
	    MechanismCapacitance(MechanismCapacitance::ParameterSpace(
	        ParameterInterval<double>(20, 20),
	        MechanismCapacitance::ParameterSpace::Parameterization(20))));

	EXPECT_FALSE(synaptic_conductance_a == synaptic_current_a);
	EXPECT_TRUE(synaptic_current_a == synaptic_current_b);

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
	EXPECT_EQ(neuron.get(compartment_a_on_neuron).get(membrane_on_compartment_a), membrane_a);

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
	EXPECT_EQ(
	    neuron.in_degree(compartment_a_on_neuron), neuron.out_degree(compartment_a_on_neuron));
	EXPECT_EQ(neuron.out_degree(compartment_a_on_neuron), 2);
	EXPECT_EQ(
	    neuron.in_degree(compartment_b_on_neuron), neuron.out_degree(compartment_b_on_neuron));
	EXPECT_EQ(neuron.out_degree(compartment_b_on_neuron), 1);
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