#include <gtest/gtest.h>

#include "grenade/vx/compute_single_mac.h"

#include "grenade/vx/io_data_map.h"
#include "halco/hicann-dls/vx/v2/chip.h"
#include "halco/hicann-dls/vx/v2/synapse.h"
#include "haldls/vx/v2/event.h"

using namespace halco::hicann_dls::vx::v2;

TEST(ComputeSingleMAC, get_spike_label)
{
	/**
	 * The label is constituted by
	 * info              bits
	 *
	 * spl1 address      14-15
	 * hemisphere           13
	 * synapse driver     6-10
	 * activation         0- 4
	 *
	 * The order presented below is:
	 * (hemisphere | activation | spl1 address | synapse driver)
	 */
	{
		SynapseDriverOnSynapseDriverBlock local_drv(8);
		SynapseDriverOnDLS drv(local_drv, SynapseDriverBlockOnDLS::top);
		grenade::vx::UInt5 value(grenade::vx::UInt5::max);

		auto const label = grenade::vx::ComputeSingleMAC::get_spike_label(drv, value);
		EXPECT_TRUE(static_cast<bool>(label));
		EXPECT_EQ(label->value(), ((0 << 13) | 1 | ((8 % 4) << 14) | ((8 / 4) << 6)));
	}
	{ // bottom hemisphere
		SynapseDriverOnSynapseDriverBlock local_drv(8);
		SynapseDriverOnDLS drv(local_drv, SynapseDriverBlockOnDLS::bottom);
		grenade::vx::UInt5 value(grenade::vx::UInt5::max);

		auto const label = grenade::vx::ComputeSingleMAC::get_spike_label(drv, value);
		EXPECT_TRUE(static_cast<bool>(label));
		EXPECT_EQ(label->value(), ((1 << 13) | 1 | ((8 % 4) << 14) | ((8 / 4) << 6)));
	}
	{ // value == 13
		SynapseDriverOnSynapseDriverBlock local_drv(24);
		SynapseDriverOnDLS drv(local_drv, SynapseDriverBlockOnDLS::top);
		grenade::vx::UInt5 value(13);

		auto const label = grenade::vx::ComputeSingleMAC::get_spike_label(drv, value);
		EXPECT_TRUE(static_cast<bool>(label));
		EXPECT_EQ(label->value(), ((0 << 13) | (32 - 13) | ((24 % 4) << 14) | ((24 / 4) << 6)));
	}
	{ // value == 0
		SynapseDriverOnSynapseDriverBlock local_drv(24);
		SynapseDriverOnDLS drv(local_drv, SynapseDriverBlockOnDLS::top);
		grenade::vx::UInt5 value(0);

		auto const label = grenade::vx::ComputeSingleMAC::get_spike_label(drv, value);
		EXPECT_FALSE(static_cast<bool>(label));
	}
}

TEST(ComputeSingleMAC, generate_input_events)
{
	grenade::vx::ComputeSingleMAC::SynramHandle synram_handle_top{0, 10, 0, HemisphereOnDLS(0)};
	grenade::vx::ComputeSingleMAC::SynramHandle synram_handle_bottom{0, 13, 10, HemisphereOnDLS(1)};
	std::vector<grenade::vx::ComputeSingleMAC::SynramHandle> synram_handles = {
	    synram_handle_top, synram_handle_bottom};

	grenade::vx::ComputeSingleMAC::Activations activations(1);
	activations.at(0).resize(25);
	for (size_t i = 0; i < activations.at(0).size(); ++i) {
		activations.at(0).at(i) = grenade::vx::UInt5(i);
	}

	size_t num_sends = 2;
	haldls::vx::v2::Timer::Value wait_between_events(3);

	auto const events = grenade::vx::ComputeSingleMAC::generate_input_events(
	    activations, synram_handles, num_sends, wait_between_events);

	EXPECT_TRUE(events.int8.empty());
	EXPECT_EQ(events.spike_events.size(), 1);

	auto const spikes = events.spike_events.at(0);
	EXPECT_EQ(spikes.size(), 1);
	EXPECT_EQ(
	    spikes.at(0).size(),
	    std::max(10 - 1 /* first activation value = 0 */, 13) * 2 /* num_sends */);

	// TODO: test the content of the spike train
}
