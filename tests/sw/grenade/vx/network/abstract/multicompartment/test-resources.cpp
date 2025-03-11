#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/capacitance.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_conductance.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_current.h"
#include "grenade/vx/network/abstract/multicompartment/neuron.h"
#include "grenade/vx/network/abstract/multicompartment/resource_manager.h"
#include "grenade/vx/network/abstract/multicompartment/synaptic_input_environment.h"
#include <iostream>
#include <gtest/gtest.h>

using namespace grenade::vx::network::abstract;

TEST(multicompartment_neuron, Resources)
{
	// Neuron and its parameter space
	Neuron neuron;
	Neuron::ParameterSpace neuron_parameter_space;

	// Compartments and their parameter_spaces;
	Compartment compartment_a;
	Compartment::ParameterSpace compartment_parameter_space_a;
	Compartment compartment_b;
	Compartment::ParameterSpace compartment_parameter_space_b;
	Compartment compartment_c;
	Compartment::ParameterSpace compartment_parameter_space_c;

	// Define mechanism parameter spaces
	MechanismCapacitance::ParameterSpace mechanism_parameter_space_capacitance_a(
	    ParameterInterval<double>(1, 25));
	MechanismCapacitance::ParameterSpace mechanism_parameter_space_capacitance_b(
	    ParameterInterval<double>(5, 12));
	MechanismSynapticInputCurrent::ParameterSpace mechanism_parameter_space_synaptic_current_a(
	    ParameterInterval<double>(1, 1), ParameterInterval<double>(2, 2));
	MechanismSynapticInputCurrent::ParameterSpace mechanism_parameter_space_synaptic_current_b(
	    ParameterInterval<double>(1, 1), ParameterInterval<double>(2, 2));
	MechanismSynapticInputConductance::ParameterSpace
	    mechanism_parameter_space_synaptic_conductance_a(
	        ParameterInterval<double>(1, 1), ParameterInterval<double>(1, 1),
	        ParameterInterval<double>(2, 2));
	MechanismSynapticInputConductance::ParameterSpace
	    mechanism_parameter_space_synaptic_conductance_b(
	        ParameterInterval<double>(1, 1), ParameterInterval<double>(2, 2),
	        ParameterInterval<double>(1, 1));

	// Define mechanisms
	MechanismCapacitance membrane_a, membrane_b;
	MechanismSynapticInputCurrent synaptic_current_a, synaptic_current_b;
	MechanismSynapticInputConductance synaptic_conductance_a, synaptic_conductance_b;

	// Add Mechanisms to Compartments
	auto const membrane_on_compartment_a = compartment_a.add(membrane_a);
	compartment_parameter_space_a.mechanisms.set(
	    membrane_on_compartment_a, mechanism_parameter_space_capacitance_a);
	auto const membrane_on_compartment_b = compartment_b.add(membrane_b);
	compartment_parameter_space_b.mechanisms.set(
	    membrane_on_compartment_b, mechanism_parameter_space_capacitance_b);
	auto const membrane_on_compartment_c = compartment_c.add(membrane_b);
	compartment_parameter_space_c.mechanisms.set(
	    membrane_on_compartment_c, mechanism_parameter_space_capacitance_b);

	auto const synaptic_current_a_on_compartment_a = compartment_a.add(synaptic_current_a);
	compartment_parameter_space_a.mechanisms.set(
	    synaptic_current_a_on_compartment_a, mechanism_parameter_space_synaptic_current_a);
	auto const synaptic_conductance_on_compartment_a = compartment_a.add(synaptic_conductance_a);
	compartment_parameter_space_a.mechanisms.set(
	    synaptic_conductance_on_compartment_a, mechanism_parameter_space_synaptic_conductance_a);
	auto const synaptic_current_b_on_compartment_b = compartment_b.add(synaptic_current_b);
	compartment_parameter_space_b.mechanisms.set(
	    synaptic_current_b_on_compartment_b, mechanism_parameter_space_synaptic_current_b);
	auto const synaptic_conductance_on_compartment_b = compartment_b.add(synaptic_conductance_b);
	compartment_parameter_space_b.mechanisms.set(
	    synaptic_conductance_on_compartment_b, mechanism_parameter_space_synaptic_conductance_b);

	// Add Compartments to Neuron
	auto const compartment_a_on_neuron = neuron.add_compartment(compartment_a);
	neuron_parameter_space.compartments.emplace(
	    compartment_a_on_neuron, compartment_parameter_space_a);
	auto const compartment_b_on_neuron = neuron.add_compartment(compartment_b);
	neuron_parameter_space.compartments.emplace(
	    compartment_b_on_neuron, compartment_parameter_space_b);
	auto const compartment_c_on_neuron = neuron.add_compartment(compartment_c);
	neuron_parameter_space.compartments.emplace(
	    compartment_c_on_neuron, compartment_parameter_space_c);

	// Add Compartment-Connections to Neuron
	CompartmentConnectionConductance connection_conductance_1;
	CompartmentConnectionConductance connection_conductance_2;
	[[maybe_unused]] auto const compartment_connection_ab_on_neuron =
	    neuron.add_compartment_connection(
	        compartment_a_on_neuron, compartment_b_on_neuron, connection_conductance_1);
	[[maybe_unused]] auto const compartment_connection_ac_on_neuron =
	    neuron.add_compartment_connection(
	        compartment_a_on_neuron, compartment_c_on_neuron, connection_conductance_2);

	// Environment No Synaptic Inputs
	Environment environment;

	// ResourceManager
	ResourceManager resource_manager;

	// Add Synaptic Inputs
	SynapticInputEnvironmentCurrent synaptic_input_current_a(true, 128);
	SynapticInputEnvironmentCurrent synaptic_input_current_b(false, 512);

	std::vector<dapr::PropertyHolder<SynapticInputEnvironment>> synaptic_input_on_compartment_a = {
	    synaptic_input_current_a, synaptic_input_current_b};
	environment.add(compartment_a_on_neuron, synaptic_input_on_compartment_a);

	SynapticInputEnvironmentConductance synaptic_input_conductance_a(true, 700);
	SynapticInputEnvironmentConductance synaptic_input_conductance_b(
	    true, NumberTopBottom(200, 100, 0));

	std::vector<dapr::PropertyHolder<SynapticInputEnvironment>> synaptic_input_on_compartment_b = {
	    synaptic_input_conductance_a, synaptic_input_conductance_b};
	environment.add(compartment_b_on_neuron, synaptic_input_on_compartment_b);


	// Add Compartment-Configuration to Resource-Manager
	resource_manager.add_config(neuron, neuron_parameter_space, environment);

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
	    resource_manager.get_config(compartment_c_on_neuron).number_total,
	    3); // 3 for Membranes
}