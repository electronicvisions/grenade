#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/event.h"
#include "halco/hicann-dls/vx/v3/event.h"

using namespace grenade::vx::signal_flow;

TEST(TimedSpikeToChip, General)
{
	TimedSpikeToChip spike(
	    grenade::vx::common::Time(123),
	    haldls::vx::v3::SpikePack1ToChip({halco::hicann_dls::vx::v3::SpikeLabel(456)}));

	EXPECT_EQ(spike.time, grenade::vx::common::Time(123));
	EXPECT_TRUE(std::holds_alternative<haldls::vx::v3::SpikePack1ToChip>(spike.data));
	EXPECT_EQ(
	    std::get<haldls::vx::v3::SpikePack1ToChip>(spike.data),
	    haldls::vx::v3::SpikePack1ToChip({halco::hicann_dls::vx::v3::SpikeLabel(456)}));

	TimedSpikeToChip spike_other = spike;
	spike_other.time = grenade::vx::common::Time(321);
	EXPECT_NE(spike, spike_other);
	spike_other.time = spike.time;
	spike_other.data =
	    haldls::vx::v3::SpikePack1ToChip({halco::hicann_dls::vx::v3::SpikeLabel(654)});
	EXPECT_NE(spike, spike_other);

	TimedSpikeToChip spike_copy = spike;
	EXPECT_EQ(spike, spike_copy);
}
