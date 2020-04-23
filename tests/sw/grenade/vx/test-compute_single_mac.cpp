#include <gtest/gtest.h>

#include "grenade/vx/compute_single_mac.h"

#include "grenade/vx/data_map.h"
#include "halco/hicann-dls/vx/chip.h"
#include "halco/hicann-dls/vx/synapse.h"
#include "haldls/vx/event.h"

using namespace halco::hicann_dls::vx;

TEST(ComputeSingleMAC, get_spike_label)
{
	/**
	 * The label is constituted by
	 * info              bits
	 *
	 * spl1 address      14-15
	 * hemisphere           13
	 * synapse driver     6-10
	 * syn row on driver     5
	 * activation         0- 4
	 *
	 * The order presented below is:
	 * (hemisphere | activation | syn row on driver | spl1 address | synapse driver)
	 */
	{ // odd local row
		SynapseRowOnSynram local_row(17);
		SynapseRowOnDLS row(local_row, SynramOnDLS::top);
		grenade::vx::UInt5 value(grenade::vx::UInt5::max);

		auto const label = grenade::vx::ComputeSingleMAC::get_spike_label(row, value);
		EXPECT_TRUE(static_cast<bool>(label));
		EXPECT_EQ(
		    label->value(),
		    ((0 << 13) | 1 | ((17 % 2) << 5) | (((17 / 2) % 4) << 14) | (((17 / 2) / 4) << 6)));
	}
	{ // bottom hemisphere
		SynapseRowOnSynram local_row(17);
		SynapseRowOnDLS row(local_row, SynramOnDLS::bottom);
		grenade::vx::UInt5 value(grenade::vx::UInt5::max);

		auto const label = grenade::vx::ComputeSingleMAC::get_spike_label(row, value);
		EXPECT_TRUE(static_cast<bool>(label));
		EXPECT_EQ(
		    label->value(),
		    ((1 << 13) | 1 | ((17 % 2) << 5) | (((17 / 2) % 4) << 14) | (((17 / 2) / 4) << 6)));
	}
	{ // even local row
		SynapseRowOnSynram local_row(48);
		SynapseRowOnDLS row(local_row, SynramOnDLS::top);
		grenade::vx::UInt5 value(grenade::vx::UInt5::max);

		auto const label = grenade::vx::ComputeSingleMAC::get_spike_label(row, value);
		EXPECT_TRUE(static_cast<bool>(label));
		EXPECT_EQ(
		    label->value(),
		    ((0 << 13) | 1 | ((48 % 2) << 5) | (((48 / 2) % 4) << 14) | (((48 / 2) / 4) << 6)));
	}
	{ // value == 13
		SynapseRowOnSynram local_row(48);
		SynapseRowOnDLS row(local_row, SynramOnDLS::top);
		grenade::vx::UInt5 value(13);

		auto const label = grenade::vx::ComputeSingleMAC::get_spike_label(row, value);
		EXPECT_TRUE(static_cast<bool>(label));
		EXPECT_EQ(
		    label->value(), ((0 << 13) | (32 - 13) | ((48 % 2) << 5) | (((48 / 2) % 4) << 14) |
		                     (((48 / 2) / 4) << 6)));
	}
	{ // value == 0
		SynapseRowOnSynram local_row(48);
		SynapseRowOnDLS row(local_row, SynramOnDLS::top);
		grenade::vx::UInt5 value(0);

		auto const label = grenade::vx::ComputeSingleMAC::get_spike_label(row, value);
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
	haldls::vx::Timer::Value wait_between_events(3);

	auto const events = grenade::vx::ComputeSingleMAC::generate_input_events(
	    activations, synram_handles, num_sends, wait_between_events);

	EXPECT_TRUE(events.int8.empty());
	EXPECT_EQ(events.spike_events.size(), 1);

	auto const spikes = events.spike_events.at(0);
	EXPECT_EQ(spikes.size(), 1);
	EXPECT_EQ(
	    spikes.at(0).size(), (10 - 1 /* first activation value = 0 */ + 13) * 2 /* num_sends */);

	// TODO: test the content of the spike train
}
