#include <gtest/gtest.h>

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/config.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/v2/chip.h"
#include "haldls/vx/v2/systime.h"
#include "logging_ctrl.h"
#include "stadls/vx/v2/init_generator.h"
#include "stadls/vx/v2/playback_generator.h"
#include "stadls/vx/v2/run.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v2;
using namespace stadls::vx::v2;
using namespace lola::vx::v2;
using namespace haldls::vx::v2;

void test_background_spike_source_regular(
    BackgroundSpikeSource::Period period,
    Timer::Value running_period,
    size_t spike_count_deviation,
    grenade::vx::JITGraphExecutor::Connections const& connections)
{
	size_t expected_count =
	    running_period * 2 /* f(FPGA) = 0.5 * f(BackgroundSpikeSource) */ / period;

	typed_array<SpikeLabel, BackgroundSpikeSourceOnDLS> expected_labels;

	grenade::vx::Graph g;

	grenade::vx::coordinate::ExecutionInstance instance;

	std::vector<grenade::vx::Input> crossbar_nodes;

	// enable background spike sources with unique configuration
	for (auto source_coord : iter_all<BackgroundSpikeSourceOnDLS>()) {
		NeuronLabel neuron_label(source_coord.toEnum());
		SPL1Address spl1_address(source_coord % SPL1Address::size);
		SpikeLabel label;
		label.set_neuron_label(neuron_label);
		label.set_spl1_address(spl1_address);
		expected_labels[source_coord] = label;

		BackgroundSpikeSource source_config;
		source_config.set_period(period);
		source_config.set_enable(true);
		source_config.set_enable_random(false);
		source_config.set_neuron_label(neuron_label);

		grenade::vx::vertex::BackgroundSpikeSource source(source_config, source_coord);
		auto const v1 = g.add(source, instance, {});

		grenade::vx::vertex::CrossbarNode crossbar_node(
		    CrossbarNodeOnDLS(
		        source_coord.toCrossbarInputOnDLS(),
		        CrossbarOutputOnDLS(8 + source_coord % CrossbarL2OutputOnDLS::size)),
		    haldls::vx::v2::CrossbarNode());

		crossbar_nodes.push_back(g.add(crossbar_node, instance, {v1}));
	}

	grenade::vx::vertex::CrossbarL2Output crossbar_output;
	grenade::vx::vertex::DataOutput data_output(
	    grenade::vx::ConnectionType::TimedSpikeFromChipSequence, 1);

	auto const v3 = g.add(crossbar_output, instance, crossbar_nodes);
	auto const v4 = g.add(data_output, instance, {v3});

	grenade::vx::IODataMap input_list;
	input_list.runtime.push_back(running_period);

	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs.insert({DLSGlobal(), grenade::vx::ChipConfig()});

	// run Graph with given inputs and return results
	auto const result_map =
	    grenade::vx::JITGraphExecutor::run(g, input_list, connections, chip_configs);

	EXPECT_EQ(result_map.data.size(), 1);

	EXPECT_TRUE(result_map.data.find(v4) != result_map.data.end());

	auto const spikes =
	    std::get<std::vector<grenade::vx::TimedSpikeFromChipSequence>>(result_map.data.at(v4))
	        .at(0);

	typed_array<size_t, BackgroundSpikeSourceOnDLS> expected_labels_count;
	expected_labels_count.fill(0);
	for (auto spike : spikes) {
		for (auto source_coord : iter_all<BackgroundSpikeSourceOnDLS>()) {
			if (expected_labels[source_coord] == spike.get_label()) {
				expected_labels_count[source_coord] += 1;
			}
		}
	}

	// for every source check approx. equality in number of spikes expected
	for (auto source_coord : iter_all<BackgroundSpikeSourceOnDLS>()) {
		EXPECT_TRUE(
		    (expected_labels_count[source_coord] <= (expected_count + spike_count_deviation)) &&
		    (expected_labels_count[source_coord] >= (expected_count - spike_count_deviation)))
		    << "expected: " << expected_count << " actual: " << expected_labels_count[source_coord];
	}
}

/**
 * Enable background sources and generate regular spike-trains.
 */
TEST(BackgroundSpikeSource, Regular)
{
	// Construct map of one connection and connect to HW
	grenade::vx::JITGraphExecutor::Connections connections;
	grenade::vx::backend::Connection connection;
	connections.insert(
	    std::pair<DLSGlobal, grenade::vx::backend::Connection&>(DLSGlobal(), connection));

	// 5% allowed deviation in spike count
	test_background_spike_source_regular(
	    BackgroundSpikeSource::Period(1000), Timer::Value(10000000), 1000, connections);
	test_background_spike_source_regular(
	    BackgroundSpikeSource::Period(10000), Timer::Value(100000000), 1000, connections);
}
