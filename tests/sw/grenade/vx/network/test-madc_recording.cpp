#include <gtest/gtest.h>

#include "grenade/vx/network/madc_recording.h"

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(network_MADCRecording, General)
{
	MADCRecording recording;
	recording.neurons.resize(1);
	recording.neurons.at(0).coordinate.population = PopulationOnExecutionInstance(12);
	recording.neurons.at(0).coordinate.neuron_on_population = 13;
	recording.neurons.at(0).coordinate.compartment_on_neuron = CompartmentOnLogicalNeuron(14);
	recording.neurons.at(0).coordinate.atomic_neuron_on_compartment = 15;
	recording.neurons.at(0).source = MADCRecording::Neuron::Source::exc_synin;

	MADCRecording recording_copy(recording);
	EXPECT_EQ(recording, recording_copy);
	recording_copy.neurons.at(0).coordinate.population = PopulationOnExecutionInstance(11);
	EXPECT_NE(recording, recording_copy);
	recording_copy.neurons.at(0).coordinate.population = PopulationOnExecutionInstance(12);
	EXPECT_EQ(recording, recording_copy);
	recording_copy.neurons.at(0).coordinate.neuron_on_population = 12;
	EXPECT_NE(recording, recording_copy);
	recording_copy.neurons.at(0).coordinate.neuron_on_population = 13;
	EXPECT_EQ(recording, recording_copy);
	recording_copy.neurons.at(0).coordinate.compartment_on_neuron = CompartmentOnLogicalNeuron(13);
	EXPECT_NE(recording, recording_copy);
	recording_copy.neurons.at(0).coordinate.compartment_on_neuron = CompartmentOnLogicalNeuron(14);
	EXPECT_EQ(recording, recording_copy);
	recording_copy.neurons.at(0).coordinate.atomic_neuron_on_compartment = 14;
	EXPECT_NE(recording, recording_copy);
	recording_copy.neurons.at(0).coordinate.atomic_neuron_on_compartment = 15;
	EXPECT_EQ(recording, recording_copy);
	recording_copy.neurons.at(0).source = MADCRecording::Neuron::Source::inh_synin;
	EXPECT_NE(recording, recording_copy);
}
