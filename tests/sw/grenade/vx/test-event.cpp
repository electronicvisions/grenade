#include <gtest/gtest.h>

#include "grenade/vx/event.h"
#include "halco/hicann-dls/vx/v3/event.h"

using namespace grenade::vx;

TEST(TimedSpike, General)
{
	TimedSpike spike(
	    TimedSpike::Time(123),
	    haldls::vx::v3::SpikePack1ToChip({halco::hicann_dls::vx::v3::SpikeLabel(456)}));

	EXPECT_EQ(spike.time, TimedSpike::Time(123));
	EXPECT_TRUE(std::holds_alternative<haldls::vx::v3::SpikePack1ToChip>(spike.payload));
	EXPECT_EQ(
	    std::get<haldls::vx::v3::SpikePack1ToChip>(spike.payload),
	    haldls::vx::v3::SpikePack1ToChip({halco::hicann_dls::vx::v3::SpikeLabel(456)}));

	TimedSpike spike_other = spike;
	spike_other.time = TimedSpike::Time(321);
	EXPECT_NE(spike, spike_other);
	spike_other.time = spike.time;
	spike_other.payload =
	    haldls::vx::v3::SpikePack1ToChip({halco::hicann_dls::vx::v3::SpikeLabel(654)});
	EXPECT_NE(spike, spike_other);

	TimedSpike spike_copy = spike;
	EXPECT_EQ(spike, spike_copy);
}
