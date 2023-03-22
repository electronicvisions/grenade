#include <gtest/gtest.h>

#include "grenade/vx/network/placed_logical/madc_recording.h"

using namespace grenade::vx::network::placed_logical;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(network_MADCRecording, General)
{
	MADCRecording recording;
	recording.population = PopulationDescriptor(12);
	recording.neuron_on_population = 13;
	recording.compartment_on_neuron = CompartmentOnLogicalNeuron(14);
	recording.atomic_neuron_on_compartment = 15;
	recording.source = MADCRecording::Source::exc_synin;

	MADCRecording recording_copy(recording);
	EXPECT_EQ(recording, recording_copy);
	recording_copy.population = PopulationDescriptor(11);
	EXPECT_NE(recording, recording_copy);
	recording_copy.population = PopulationDescriptor(12);
	EXPECT_EQ(recording, recording_copy);
	recording_copy.neuron_on_population = 12;
	EXPECT_NE(recording, recording_copy);
	recording_copy.neuron_on_population = 13;
	EXPECT_EQ(recording, recording_copy);
	recording_copy.compartment_on_neuron = CompartmentOnLogicalNeuron(13);
	EXPECT_NE(recording, recording_copy);
	recording_copy.compartment_on_neuron = CompartmentOnLogicalNeuron(14);
	EXPECT_EQ(recording, recording_copy);
	recording_copy.atomic_neuron_on_compartment = 14;
	EXPECT_NE(recording, recording_copy);
	recording_copy.atomic_neuron_on_compartment = 15;
	EXPECT_EQ(recording, recording_copy);
	recording_copy.source = MADCRecording::Source::inh_synin;
	EXPECT_NE(recording, recording_copy);
}
