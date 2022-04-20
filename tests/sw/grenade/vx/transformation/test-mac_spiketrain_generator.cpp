#include <gtest/gtest.h>

#include "grenade/vx/transformation/mac_spiketrain_generator.h"

#include "grenade/vx/io_data_map.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "haldls/vx/v3/event.h"

using namespace halco::hicann_dls::vx::v3;

TEST(MACSpikeTrainGenerator, get_spike_label)
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

		auto const label =
		    grenade::vx::transformation::MACSpikeTrainGenerator::get_spike_label(drv, value);
		EXPECT_TRUE(static_cast<bool>(label));
		EXPECT_EQ(label->value(), ((0 << 13) | 1 | ((8 % 4) << 14) | ((8 / 4) << 6)));
	}
	{ // bottom hemisphere
		SynapseDriverOnSynapseDriverBlock local_drv(8);
		SynapseDriverOnDLS drv(local_drv, SynapseDriverBlockOnDLS::bottom);
		grenade::vx::UInt5 value(grenade::vx::UInt5::max);

		auto const label =
		    grenade::vx::transformation::MACSpikeTrainGenerator::get_spike_label(drv, value);
		EXPECT_TRUE(static_cast<bool>(label));
		EXPECT_EQ(label->value(), ((1 << 13) | 1 | ((8 % 4) << 14) | ((8 / 4) << 6)));
	}
	{ // value == 13
		SynapseDriverOnSynapseDriverBlock local_drv(24);
		SynapseDriverOnDLS drv(local_drv, SynapseDriverBlockOnDLS::top);
		grenade::vx::UInt5 value(13);

		auto const label =
		    grenade::vx::transformation::MACSpikeTrainGenerator::get_spike_label(drv, value);
		EXPECT_TRUE(static_cast<bool>(label));
		EXPECT_EQ(label->value(), ((0 << 13) | (32 - 13) | ((24 % 4) << 14) | ((24 / 4) << 6)));
	}
	{ // value == 0
		SynapseDriverOnSynapseDriverBlock local_drv(24);
		SynapseDriverOnDLS drv(local_drv, SynapseDriverBlockOnDLS::top);
		grenade::vx::UInt5 value(0);

		auto const label =
		    grenade::vx::transformation::MACSpikeTrainGenerator::get_spike_label(drv, value);
		EXPECT_FALSE(static_cast<bool>(label));
	}
}

TEST(MACSpikeTrainGenerator, apply)
{
	size_t hemisphere_size_top = 10;
	size_t hemisphere_size_bot = 13;
	std::vector<grenade::vx::TimedDataSequence<std::vector<grenade::vx::UInt5>>> activations_top(1);
	std::vector<grenade::vx::TimedDataSequence<std::vector<grenade::vx::UInt5>>> activations_bot(1);
	activations_top.at(0).resize(1);
	activations_bot.at(0).resize(1);
	activations_top.at(0).at(0).data.resize(hemisphere_size_top);
	activations_bot.at(0).at(0).data.resize(hemisphere_size_bot);
	for (size_t i = 0; i < activations_top.at(0).at(0).data.size(); ++i) {
		activations_top.at(0).at(0).data.at(i) = grenade::vx::UInt5(i);
	}
	for (size_t i = 0; i < activations_bot.at(0).at(0).data.size(); ++i) {
		activations_bot.at(0).at(0).data.at(i) = grenade::vx::UInt5(i + hemisphere_size_top);
	}

	size_t num_sends = 2;
	haldls::vx::v3::Timer::Value wait_between_events(3);

	grenade::vx::transformation::MACSpikeTrainGenerator generator(
	    {hemisphere_size_top, hemisphere_size_bot}, num_sends, wait_between_events);
	std::vector<grenade::vx::transformation::MACSpikeTrainGenerator::Value> inputs = {
	    activations_top, activations_bot};
	auto const events = generator.apply(inputs);

	EXPECT_TRUE(std::holds_alternative<std::vector<grenade::vx::TimedSpikeSequence>>(events));
	auto const& vevents = std::get<std::vector<grenade::vx::TimedSpikeSequence>>(events);
	EXPECT_EQ(vevents.size(), 1);

	auto const spikes = vevents.at(0);
	EXPECT_EQ(
	    spikes.size(), std::max(10 - 1 /* first activation value = 0 */, 13) * 2 /* num_sends */);

	// TODO: test the content of the spike train
}
