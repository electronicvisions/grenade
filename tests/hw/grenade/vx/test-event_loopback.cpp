#include <gtest/gtest.h>

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/topology.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_input.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "halco/hicann-dls/vx/v3/event.h"
#include "haldls/vx/v3/systime.h"
#include "lola/vx/v3/chip.h"
#include "stadls/vx/v3/init_generator.h"
#include "stadls/vx/v3/playback_generator.h"
#include "stadls/vx/v3/run.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace stadls::vx::v3;
using namespace lola::vx::v3;

std::vector<grenade::vx::signal_flow::TimedSpikeFromChipSequence>
test_event_loopback_single_crossbar_node(
    CrossbarL2OutputOnDLS const& node,
    grenade::vx::execution::JITGraphExecutor& executor,
    std::vector<grenade::vx::signal_flow::TimedSpikeToChipSequence> const& inputs)
{
	grenade::common::ExecutionInstanceOnExecutor instance;

	grenade::vx::signal_flow::vertex::CrossbarL2Input crossbar_l2_input(
	    true, grenade::vx::common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(),
	    instance);

	grenade::vx::signal_flow::vertex::CrossbarNode crossbar_node(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8 + node.toEnum()), node.toCrossbarOutputOnDLS()),
	    grenade::vx::common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(), instance);

	grenade::vx::signal_flow::vertex::CrossbarL2Output crossbar_output(
	    true, grenade::vx::common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(),
	    instance);

	auto topology = std::make_shared<grenade::common::Topology>();

	auto const v1 = topology->add_vertex(crossbar_l2_input);
	auto const v2 = topology->add_vertex(crossbar_node);
	auto const v3 = topology->add_vertex(crossbar_output);

	topology->add_edge(
	    v1, v2,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
	topology->add_edge(
	    v2, v3,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));


	// fill inputs
	grenade::common::InputData input_data;
	input_data.ports.set(
	    {v1, 0}, grenade::vx::signal_flow::vertex::CrossbarL2Input::Dynamics(inputs));
	input_data.time_domain_runtimes.set(
	    grenade::common::TimeDomainOnTopology(),
	    grenade::vx::network::abstract::ClockCycleTimeDomainRuntimes(
	        std::vector<std::optional<grenade::vx::common::Time>>(inputs.size(), std::nullopt),
	        grenade::vx::common::Time()));

	input_data.ports.set(
	    {v2, 1}, grenade::vx::signal_flow::vertex::CrossbarNode::Parameterization(
	                 haldls::vx::v3::CrossbarNode()));

	// run Graph with given inputs and return results
	auto const results = grenade::vx::execution::run(executor, topology, input_data);

	EXPECT_EQ(results.batch_size(), inputs.size());

	EXPECT_TRUE(results.ports.contains({v3, 0}));
	auto const& result =
	    dynamic_cast<grenade::vx::signal_flow::vertex::CrossbarL2Output::Results const&>(
	        results.ports.get({v3, 0}));
	EXPECT_EQ(result.spikes.size(), inputs.size());

	EXPECT_EQ(result.spikes.size(), inputs.size());
	return result.spikes;
}

TEST(JITGraphExecutor, EventLoopback)
{
	// Construct map of one connection and connect to HW
	grenade::vx::execution::JITGraphExecutor executor;

	constexpr size_t max_batch_size = 5;

	for (size_t b = 1; b < max_batch_size; ++b) {
		for (auto const output : iter_all<CrossbarL2OutputOnDLS>()) {
			for (auto const address : iter_all<SPL1Address>()) {
				halco::hicann_dls::vx::v3::SpikeLabel label;
				label.set_spl1_address(address);
				std::vector<grenade::vx::signal_flow::TimedSpikeToChipSequence> inputs(b);
				for (auto& in : inputs) {
					for (intmax_t i = 0; i < 1000; ++i) {
						in.push_back(grenade::vx::signal_flow::TimedSpikeToChip{
						    grenade::vx::common::Time(i * 10),
						    grenade::vx::signal_flow::TimedSpikeToChip::Data(
						        haldls::vx::v3::SpikePack1ToChip({label}))});
					}
				}

				auto const spike_batches =
				    test_event_loopback_single_crossbar_node(output, executor, inputs);
				if (output.toEnum() == address.toEnum()) {
					// node matches input channel, events should be propagated
					for (auto const& spikes : spike_batches) {
						EXPECT_LE(spikes.size(), 1100);
						EXPECT_GE(spikes.size(), 900);
					}
				} else {
					// node does not match input channel, events should not be propagated
					for (auto const& spikes : spike_batches) {
						EXPECT_LE(spikes.size(), 100);
					}
				}
			}
		}
	}
}
