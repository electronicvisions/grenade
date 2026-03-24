#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/capacitance.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_conductance.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_current.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_environment/synaptic_input.h"
#include "grenade/vx/network/abstract/multicompartment/neuron.h"
#include "grenade/vx/network/abstract/multicompartment/resource_manager.h"
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
	EXPECT_NO_THROW(
	    MechanismCapacitance::ParameterSpace({ParameterInterval<ccalix::CapacitanceInFarad>(
	        ccalix::CapacitanceInFarad(0.2 * 2.2e-12), ccalix::CapacitanceInFarad(2. * 2.2e-12))}));
	EXPECT_NO_THROW(
	    MechanismCapacitance::ParameterSpace({ParameterInterval<ccalix::CapacitanceInFarad>(
	        ccalix::CapacitanceInFarad(1. * 2.2e-12), ccalix::CapacitanceInFarad(4. * 2.2e-12))}));
	EXPECT_NO_THROW(MechanismSynapticInputCurrent::ParameterSpace(
	    {ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue>(
	        lola::vx::v3::AtomicNeuron::AnalogValue(1),
	        lola::vx::v3::AtomicNeuron::AnalogValue(1))},
	    {ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue>(
	        lola::vx::v3::AtomicNeuron::AnalogValue(1),
	        lola::vx::v3::AtomicNeuron::AnalogValue(1))},
	    {ParameterInterval<ccalix::TimeInS>(ccalix::TimeInS(2), ccalix::TimeInS(2))}));
	EXPECT_NO_THROW(MechanismSynapticInputCurrent::ParameterSpace(
	    {ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue>(
	        lola::vx::v3::AtomicNeuron::AnalogValue(1),
	        lola::vx::v3::AtomicNeuron::AnalogValue(1))},
	    {ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue>(
	        lola::vx::v3::AtomicNeuron::AnalogValue(1),
	        lola::vx::v3::AtomicNeuron::AnalogValue(1))},
	    {ParameterInterval<ccalix::TimeInS>(ccalix::TimeInS(2), ccalix::TimeInS(2))}));
	EXPECT_NO_THROW(MechanismSynapticInputConductance::ParameterSpace(
	    {ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue>(
	        lola::vx::v3::AtomicNeuron::AnalogValue(1),
	        lola::vx::v3::AtomicNeuron::AnalogValue(1))},
	    {ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue>(
	        lola::vx::v3::AtomicNeuron::AnalogValue(1),
	        lola::vx::v3::AtomicNeuron::AnalogValue(1))},
	    {ParameterInterval<double>(1, 1)}, {ParameterInterval<std::optional<double>>(2, 2)},
	    {ParameterInterval<ccalix::TimeInS>(ccalix::TimeInS(2), ccalix::TimeInS(2))}));

	// Invalid parameter spaces
	EXPECT_THROW(
	    MechanismCapacitance::ParameterSpace({ParameterInterval<ccalix::CapacitanceInFarad>(
	        ccalix::CapacitanceInFarad(3. * 2.2e-12), ccalix::CapacitanceInFarad(1. * 2.2e-12))}),
	    std::invalid_argument);
	EXPECT_THROW(
	    MechanismSynapticInputCurrent::ParameterSpace(
	        {ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue>(
	            lola::vx::v3::AtomicNeuron::AnalogValue(1),
	            lola::vx::v3::AtomicNeuron::AnalogValue(1))},
	        {ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue>(
	            lola::vx::v3::AtomicNeuron::AnalogValue(4),
	            lola::vx::v3::AtomicNeuron::AnalogValue(1))},
	        {ParameterInterval<ccalix::TimeInS>(ccalix::TimeInS(2), ccalix::TimeInS(2))}),
	    std::invalid_argument);

	// Add Mechanisms to Compartments
	[[maybe_unused]] auto const membrane_on_compartment_a =
	    compartment_a.mechanisms.insert(membrane);
	[[maybe_unused]] auto const membrane_on_compartment_b =
	    compartment_b.mechanisms.insert(membrane);
	[[maybe_unused]] auto const membrane_on_compartment_c =
	    compartment_c.mechanisms.insert(membrane);
	[[maybe_unused]] auto const synaptic_current_a_on_compartment_a =
	    compartment_a.mechanisms.insert(synaptic_current);
	[[maybe_unused]] auto const synaptic_conductance_on_compartment_a =
	    compartment_a.mechanisms.insert(synaptic_conductance);
	[[maybe_unused]] auto const synaptic_current_b_on_compartment_b =
	    compartment_b.mechanisms.insert(synaptic_current);

	// Not two mechanisms of same type on one compartment
	EXPECT_THROW(compartment_a.mechanisms.insert(membrane), std::invalid_argument);

	compartment_c.mechanisms.set(membrane_on_compartment_c, MechanismCapacitance());
	MechanismCapacitance::ParameterSpace({ParameterInterval<ccalix::CapacitanceInFarad>(
	    ccalix::CapacitanceInFarad(4. * 2.2e-12), ccalix::CapacitanceInFarad(4. * 2.2e-12))});

	EXPECT_FALSE(synaptic_conductance == synaptic_current);
	EXPECT_TRUE(synaptic_current == synaptic_current);

	compartment_a.mechanisms.erase(synaptic_conductance_on_compartment_a);
	compartment_a.mechanisms.erase(synaptic_current_a_on_compartment_a);
	compartment_b.mechanisms.erase(synaptic_current_b_on_compartment_b);
	compartment_b.mechanisms.erase(membrane_on_compartment_b);

	EXPECT_THROW(compartment_b.mechanisms.get(membrane_on_compartment_b), std::out_of_range);

	// Add Compartments to Neuron
	auto const compartment_a_on_neuron = neuron.add_compartment(compartment_a);
	auto const compartment_b_on_neuron = neuron.add_compartment(compartment_b);
	auto const compartment_c_on_neuron = neuron.add_compartment(compartment_c);

	EXPECT_EQ(neuron.num_compartments(), 3);
	EXPECT_EQ(neuron.get(compartment_a_on_neuron), compartment_a);
	EXPECT_EQ(
	    neuron.get(compartment_a_on_neuron).mechanisms.get(membrane_on_compartment_a), membrane);

	// Add Compartment-Connections to Neuron
	CompartmentConnectionConductance connection_conductance_1;
	CompartmentConnectionConductance connection_conductance_2;

	EXPECT_EQ(connection_conductance_2, connection_conductance_2);

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


TEST(multicompartment_neuron, MorphologyComparison)
{
	// Create chain of compartments
	auto create_neuron = []() -> Neuron {
		Neuron neuron;
		Compartment compartment;

		auto const comp_id_0 = neuron.add_compartment(compartment);
		auto const comp_id_1 = neuron.add_compartment(compartment);
		auto const comp_id_2 = neuron.add_compartment(compartment);
		auto const comp_id_3 = neuron.add_compartment(compartment);

		CompartmentConnectionConductance connection_conductance;

		neuron.add_compartment_connection(comp_id_0, comp_id_1, connection_conductance);
		neuron.add_compartment_connection(comp_id_1, comp_id_2, connection_conductance);
		neuron.add_compartment_connection(comp_id_2, comp_id_3, connection_conductance);
		return neuron;
	};

	auto neuron = create_neuron();
	EXPECT_TRUE(neuron.has_equal_morphology(neuron));

	auto equal_neuron = create_neuron();
	EXPECT_TRUE(neuron.has_equal_morphology(equal_neuron));

	// empty neuron
	{
		Neuron other;
		EXPECT_FALSE(neuron.has_equal_morphology(other));
		EXPECT_TRUE(other.has_equal_morphology(other));
	}

	// isomorphic but different numbering
	{
		Neuron other;
		Compartment compartment;

		auto const comp_id_0 = other.add_compartment(compartment);
		auto const comp_id_1 = other.add_compartment(compartment);
		auto const comp_id_2 = other.add_compartment(compartment);
		auto const comp_id_3 = other.add_compartment(compartment);

		CompartmentConnectionConductance connection_conductance;

		other.add_compartment_connection(comp_id_1, comp_id_2, connection_conductance);
		other.add_compartment_connection(comp_id_2, comp_id_3, connection_conductance);
		other.add_compartment_connection(comp_id_3, comp_id_0, connection_conductance);
		EXPECT_FALSE(neuron.has_equal_morphology(other));
	}

	// non-isomorphic
	{
		Neuron other;
		Compartment compartment;

		auto const comp_id_0 = other.add_compartment(compartment);
		auto const comp_id_1 = other.add_compartment(compartment);
		auto const comp_id_2 = other.add_compartment(compartment);
		auto const comp_id_3 = other.add_compartment(compartment);

		CompartmentConnectionConductance connection_conductance;

		other.add_compartment_connection(comp_id_0, comp_id_1, connection_conductance);
		other.add_compartment_connection(comp_id_1, comp_id_2, connection_conductance);
		other.add_compartment_connection(comp_id_1, comp_id_3, connection_conductance);
		EXPECT_FALSE(neuron.has_equal_morphology(other));
	}

	// neuron with just one compartment (no connections)
	{
		Neuron other;
		Compartment compartment;

		other.add_compartment(compartment);
		EXPECT_FALSE(neuron.has_equal_morphology(other));
		EXPECT_TRUE(other.has_equal_morphology(other));
	}
}
