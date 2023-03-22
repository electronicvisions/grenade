#include <gtest/gtest.h>

#include "grenade/vx/network/placed_logical/madc_recording.h"
#include "grenade/vx/network/placed_logical/network_builder.h"

using namespace grenade::vx::network::placed_logical;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(NetworkBuilder, General)
{
	NetworkBuilder builder;

	Population population({
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS(Enum(0))),
	        Population::Neuron::Compartments{
	            {CompartmentOnLogicalNeuron(),
	             Population::Neuron::Compartment{
	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}),
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS(Enum(1))),
	        Population::Neuron::Compartments{
	            {CompartmentOnLogicalNeuron(),
	             Population::Neuron::Compartment{
	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}),
	});

	auto const descriptor = builder.add(population);

	MADCRecording madc_recording;
	madc_recording.population = descriptor;
	madc_recording.neuron_on_population = 0;

	// population present, no MADC recording added yet
	EXPECT_NO_THROW(builder.add(madc_recording));

	// only one MADC recording per network allowed
	EXPECT_THROW(builder.add(madc_recording), std::runtime_error);

	// done resets builder
	builder.done();

	builder.add(population);
	// population present, no MADC recording added yet
	EXPECT_NO_THROW(builder.add(madc_recording));

	builder.done();

	builder.add(population);
	madc_recording.neuron_on_population = 2;
	// index out of range
	EXPECT_THROW(builder.add(madc_recording), std::runtime_error);

	madc_recording.neuron_on_population = 0;
	madc_recording.population = PopulationDescriptor(123);
	// population unknown
	EXPECT_THROW(builder.add(madc_recording), std::runtime_error);

	madc_recording.population = descriptor;
	// population present, no MADC recording added yet
	EXPECT_NO_THROW(builder.add(madc_recording));
}
