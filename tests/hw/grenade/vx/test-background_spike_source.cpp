#include <gtest/gtest.h>

#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/input_data.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/common/time.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/background_spike_source.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include "halco/hicann-dls/vx/event.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/systime.h"
#include "lola/vx/v3/chip.h"
#include "stadls/vx/v3/init_generator.h"
#include "stadls/vx/v3/playback_generator.h"
#include "stadls/vx/v3/run.h"
#include <memory>

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace stadls::vx::v3;
using namespace lola::vx::v3;
using namespace haldls::vx::v3;

inline void test_background_spike_source_regular(
    BackgroundSpikeSource::Period period,
    grenade::vx::common::Time running_period,
    size_t spike_count_deviation,
    grenade::vx::execution::JITGraphExecutor& executor)
{
	size_t expected_count =
	    running_period * 2 /* f(FPGA) = 0.5 * f(BackgroundSpikeSource) */ / period;

	typed_array<SpikeLabel, BackgroundSpikeSourceOnDLS> expected_labels;

	auto topology = std::make_shared<grenade::common::Topology>();
	grenade::common::InputData input_data;

	grenade::common::ExecutionInstanceOnExecutor instance;

	std::vector<grenade::common::VertexOnTopology> crossbar_nodes;

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

		grenade::vx::signal_flow::vertex::BackgroundSpikeSource source(
		    source_coord, grenade::vx::common::ChipOnConnection(),
		    grenade::common::TimeDomainOnTopology(), instance);
		auto const v1 = topology->add_vertex(source);
		input_data.ports.set(
		    {v1, 0}, grenade::vx::signal_flow::vertex::BackgroundSpikeSource::Parameterization(
		                 source_config));

		grenade::vx::signal_flow::vertex::CrossbarNode crossbar_node(
		    CrossbarNodeOnDLS(
		        source_coord.toCrossbarInputOnDLS(),
		        CrossbarOutputOnDLS(8 + source_coord % CrossbarL2OutputOnDLS::size)),
		    grenade::vx::common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(),
		    instance);

		auto const v2 = topology->add_vertex(crossbar_node);
		input_data.ports.set(
		    {v2, 1}, grenade::vx::signal_flow::vertex::CrossbarNode::Parameterization(
		                 haldls::vx::v3::CrossbarNode()));
		crossbar_nodes.push_back(v2);
	}

	grenade::vx::signal_flow::vertex::CrossbarL2Output crossbar_output(
	    true, grenade::vx::common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(),
	    instance);

	auto const v3 = topology->add_vertex(crossbar_output);

	for (auto const& v2 : crossbar_nodes) {
		topology->add_edge(
		    v2, v3,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence({1})));
	}

	input_data.time_domain_runtimes.set(
	    grenade::common::TimeDomainOnTopology(),
	    grenade::vx::network::abstract::ClockCycleTimeDomainRuntimes(
	        {running_period}, grenade::vx::common::Time()));

	// run Graph with given inputs and return results
	auto const results = grenade::vx::execution::run(executor, topology, input_data);

	EXPECT_EQ(results.batch_size(), 1);

	EXPECT_TRUE(results.ports.contains({v3, 0}));

	auto const spikes =
	    dynamic_cast<grenade::vx::signal_flow::vertex::CrossbarL2Output::Results const&>(
	        results.ports.get({v3, 0}))
	        .spikes.at(0);

	typed_array<size_t, BackgroundSpikeSourceOnDLS> expected_labels_count;
	expected_labels_count.fill(0);
	for (auto spike : spikes) {
		for (auto source_coord : iter_all<BackgroundSpikeSourceOnDLS>()) {
			if (expected_labels[source_coord] == spike.data) {
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
	grenade::vx::execution::JITGraphExecutor executor;

	// 5% allowed deviation in spike count
	test_background_spike_source_regular(
	    BackgroundSpikeSource::Period(1000), grenade::vx::common::Time(10000000), 1000, executor);
	test_background_spike_source_regular(
	    BackgroundSpikeSource::Period(10000), grenade::vx::common::Time(100000000), 1000, executor);
}
