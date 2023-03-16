#include <gtest/gtest.h>

#include "grenade/vx/network/placed_atomic/madc_recording.h"

using namespace grenade::vx::network::placed_atomic;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(network_MADCRecording, General)
{
	MADCRecording recording(PopulationDescriptor(12), 13, MADCRecording::Source::exc_synin);

	MADCRecording recording_copy(recording);
	EXPECT_EQ(recording, recording_copy);
	recording_copy.population = PopulationDescriptor(11);
	EXPECT_NE(recording, recording_copy);
	recording_copy.population = PopulationDescriptor(12);
	EXPECT_EQ(recording, recording_copy);
	recording_copy.index = 12;
	EXPECT_NE(recording, recording_copy);
	recording_copy.index = 13;
	EXPECT_EQ(recording, recording_copy);
	recording_copy.source = MADCRecording::Source::inh_synin;
	EXPECT_NE(recording, recording_copy);
}
