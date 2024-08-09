#include <gtest/gtest.h>

#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input.h"
#include "grenade/vx/signal_flow/input_data.h"
#include "grenade/vx/signal_flow/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "halco/hicann-dls/vx/v3/event.h"
#include "haldls/vx/v3/systime.h"
#include "logging_ctrl.h"
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
	grenade::vx::signal_flow::vertex::ExternalInput external_input(
	    grenade::vx::signal_flow::ConnectionType::DataTimedSpikeToChipSequence, 1);

	grenade::vx::signal_flow::vertex::DataInput data_input(
	    grenade::vx::signal_flow::ConnectionType::TimedSpikeToChipSequence, 1);

	grenade::vx::signal_flow::vertex::CrossbarL2Input crossbar_l2_input;

	grenade::vx::signal_flow::vertex::CrossbarNode crossbar_node(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8 + node.toEnum()), node.toCrossbarOutputOnDLS()),
	    haldls::vx::v3::CrossbarNode());

	grenade::vx::signal_flow::vertex::CrossbarL2Output crossbar_output;
	grenade::vx::signal_flow::vertex::DataOutput data_output(
	    grenade::vx::signal_flow::ConnectionType::TimedSpikeFromChipSequence, 1);

	grenade::vx::signal_flow::Graph g;

	grenade::common::ExecutionInstanceID instance;

	auto const v1 = g.add(external_input, instance, {});
	auto const v2 = g.add(data_input, instance, {v1});
	auto const v3 = g.add(crossbar_l2_input, instance, {v2});
	auto const v4 = g.add(crossbar_node, instance, {v3});
	auto const v5 = g.add(crossbar_output, instance, {v4});
	auto const v6 = g.add(data_output, instance, {v5});

	// fill graph inputs
	grenade::vx::signal_flow::InputData input_list;
	input_list.data[v1] = inputs;

	grenade::vx::execution::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs.insert({instance, lola::vx::v3::Chip()});

	// run Graph with given inputs and return results
	auto const result_map = grenade::vx::execution::run(executor, g, chip_configs, input_list);

	EXPECT_EQ(result_map.data.size(), 1);

	EXPECT_TRUE(result_map.data.find(v6) != result_map.data.end());
	EXPECT_EQ(
	    std::get<std::vector<grenade::vx::signal_flow::TimedSpikeFromChipSequence>>(
	        result_map.data.at(v6))
	        .size(),
	    inputs.size());
	return std::get<std::vector<grenade::vx::signal_flow::TimedSpikeFromChipSequence>>(
	    result_map.data.at(v6));
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
