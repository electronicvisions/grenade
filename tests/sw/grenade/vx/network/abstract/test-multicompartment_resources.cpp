#include "grenade/vx/network/abstract/multicompartment_environment.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism/capacitance.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism/synaptic_conductance.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism/synaptic_current.h"
#include "grenade/vx/network/abstract/multicompartment_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_resource_manager.h"
#include "grenade/vx/network/abstract/multicompartment_synaptic_input_environment.h"
#include <iostream>
#include <gtest/gtest.h>

using namespace grenade::vx::network::abstract;

TEST(multicompartment_neuron, Resources)
{
	// Neuron
	Neuron neuron;

	// Compartments
	Compartment compartment_a;
	Compartment compartment_b;
	Compartment compartment_c;

	// Mechanisms
	MechanismCapacitance membrane_a(MechanismCapacitance::ParameterSpace(
	    ParameterInterval<double>(1, 25),
	    MechanismCapacitance::ParameterSpace::Parameterization(16)));
	MechanismCapacitance membrane_b(MechanismCapacitance::ParameterSpace(
	    ParameterInterval<double>(5, 12),
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
	MechanismSynapticInputConductance synaptic_conductance_b(
	    MechanismSynapticInputConductance::ParameterSpace(
	        ParameterInterval<double>(1, 1), ParameterInterval<double>(2, 2),
	        ParameterInterval<double>(1, 1),
	        MechanismSynapticInputConductance::ParameterSpace::Parameterization(1, 2, 1)));

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
	[[maybe_unused]] auto const synaptic_conductance_on_compartment_b =
	    compartment_b.add(synaptic_conductance_b);

	// Add Compartments to Neuron
	[[maybe_unused]] auto const compartment_a_on_neuron = neuron.add_compartment(compartment_a);
	[[maybe_unused]] auto const compartment_b_on_neuron = neuron.add_compartment(compartment_b);
	[[maybe_unused]] auto const compartment_c_on_Neuron = neuron.add_compartment(compartment_c);

	// Add Compartment-Connections to Neuron
	CompartmentConnectionConductance connection_conductance_1;
	CompartmentConnectionConductance connection_conductance_2;
	[[maybe_unused]] auto const compartment_connection_ab_on_neuron =
	    neuron.add_compartment_connection(
	        compartment_a_on_neuron, compartment_b_on_neuron, connection_conductance_1);
	[[maybe_unused]] auto const compartment_connection_ac_on_neuron =
	    neuron.add_compartment_connection(
	        compartment_a_on_neuron, compartment_c_on_Neuron, connection_conductance_2);

	// Environment No Synaptic Inputs
	Environment environment;

	// ResourceManager
	ResourceManager resource_manager;

	// Add Synaptic Inputs
	SynapticInputEnvironmentCurrent synaptic_input_current_a(true, 128);
	SynapticInputEnvironmentCurrent synaptic_input_current_b(false, 512);

	std::vector<grenade::common::detail::PropertyHolder<SynapticInputEnvironment>>
	    synaptic_input_on_compartment_a = {synaptic_input_current_a, synaptic_input_current_b};
	environment.synaptic_connections.emplace(
	    compartment_a_on_neuron, synaptic_input_on_compartment_a);

	SynapticInputEnvironmentConductance synaptic_input_conductance_a(true, 700);
	SynapticInputEnvironmentConductance synaptic_input_conductance_b(
	    true, NumberTopBottom(200, 100, 0));

	std::vector<grenade::common::detail::PropertyHolder<SynapticInputEnvironment>>
	    synaptic_input_on_compartment_b = {
	        synaptic_input_conductance_a, synaptic_input_conductance_b};
	environment.synaptic_connections.emplace(
	    compartment_b_on_neuron, synaptic_input_on_compartment_b);


	// Add Compartment-Configuration to Resource-Manager
	resource_manager.add_config(compartment_a_on_neuron, neuron, environment);
	resource_manager.add_config(compartment_b_on_neuron, neuron, environment);
	resource_manager.add_config(compartment_c_on_Neuron, neuron, environment);

	EXPECT_EQ(
	    resource_manager.get_config(compartment_a_on_neuron).number_total,
	    5); // 5 for Membranes and 3 for Synaptic Inputs
	EXPECT_EQ(
	    resource_manager.get_config(compartment_b_on_neuron).number_total,
	    4); // 3 for Membranes and 4 for Synaptic Inputs
	EXPECT_EQ(
	    resource_manager.get_config(compartment_b_on_neuron).number_top,
	    1); // 1 fof synaptic_input_conductance_b
	EXPECT_EQ(
	    resource_manager.get_config(compartment_c_on_Neuron).number_total,
	    3); // 3 for Membranes
}