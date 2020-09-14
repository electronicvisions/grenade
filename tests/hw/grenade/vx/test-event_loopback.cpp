#include <gtest/gtest.h>

#include "grenade/vx/config.h"
#include "grenade/vx/data_map.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/v2/chip.h"
#include "haldls/vx/v2/systime.h"
#include "hxcomm/vx/connection_from_env.h"
#include "logging_ctrl.h"
#include "stadls/vx/v2/init_generator.h"
#include "stadls/vx/v2/playback_generator.h"
#include "stadls/vx/v2/run.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v2;
using namespace stadls::vx::v2;
using namespace lola::vx::v2;

std::vector<grenade::vx::TimedSpikeFromChipSequence> test_event_loopback_single_crossbar_node(
    CrossbarL2OutputOnDLS const& node,
    grenade::vx::JITGraphExecutor::ExecutorMap const& executors,
    std::vector<grenade::vx::TimedSpikeSequence> const& inputs)
{
	logger_default_config(log4cxx::Level::getTrace());

	grenade::vx::vertex::ExternalInput external_input(
	    grenade::vx::ConnectionType::DataInputUInt16, 1);

	grenade::vx::vertex::DataInput data_input(grenade::vx::ConnectionType::CrossbarInputLabel, 1);

	grenade::vx::vertex::CrossbarNode crossbar_node(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8 + node.toEnum()), node.toCrossbarOutputOnDLS()),
	    haldls::vx::v2::CrossbarNode());

	grenade::vx::vertex::CrossbarL2Output crossbar_output;
	grenade::vx::vertex::DataOutput data_output(grenade::vx::ConnectionType::DataOutputUInt16, 1);

	grenade::vx::Graph g;

	grenade::vx::coordinate::ExecutionInstance instance;

	auto const v1 = g.add(external_input, instance, {});
	auto const v2 = g.add(data_input, instance, {v1});
	auto const v3 = g.add(crossbar_node, instance, {v2});
	auto const v4 = g.add(crossbar_output, instance, {v3});
	auto const v5 = g.add(data_output, instance, {v4});

	// fill graph inputs
	grenade::vx::DataMap input_list;
	input_list.spike_events[v1] = inputs;

	grenade::vx::JITGraphExecutor::ConfigMap config_map;
	config_map.insert({DLSGlobal(), grenade::vx::ChipConfig()});

	// run Graph with given inputs and return results
	auto const result_map =
	    grenade::vx::JITGraphExecutor::run(g, input_list, executors, config_map);

	EXPECT_EQ(result_map.spike_event_output.size(), 1);

	EXPECT_TRUE(result_map.spike_event_output.find(v5) != result_map.spike_event_output.end());
	EXPECT_EQ(result_map.spike_event_output.at(v5).size(), inputs.size());
	return result_map.spike_event_output.at(v5);
}

TEST(JITGraphExecutor, EventLoopback)
{
	// Construct map of one executor and connect to HW
	grenade::vx::JITGraphExecutor::ExecutorMap executors;
	auto connection = hxcomm::vx::get_connection_from_env();
	executors.insert(std::pair<DLSGlobal, hxcomm::vx::ConnectionVariant&>(DLSGlobal(), connection));

	// Initialize chip
	{
		DigitalInit const init;
		auto [builder, _] = generate(init);
		auto program = builder.done();
		stadls::vx::v2::run(executors.at(DLSGlobal()), program);
	}

	constexpr size_t max_batch_size = 5;

	for (size_t b = 1; b < max_batch_size; ++b) {
		for (auto const output : iter_all<CrossbarL2OutputOnDLS>()) {
			for (auto const address : iter_all<SPL1Address>()) {
				haldls::vx::v2::SpikeLabel label;
				label.set_spl1_address(address);
				std::vector<grenade::vx::TimedSpikeSequence> inputs(b);
				for (auto& in : inputs) {
					for (intmax_t i = 0; i < 1000; ++i) {
						in.push_back(grenade::vx::TimedSpike{
						    grenade::vx::TimedSpike::Time(i * 10),
						    grenade::vx::TimedSpike::Payload(
						        haldls::vx::v2::SpikePack1ToChip({label}))});
					}
				}

				auto const spike_batches =
				    test_event_loopback_single_crossbar_node(output, executors, inputs);
				if (output.toEnum() == address.toEnum()) {
					// node matches input channel, events should be propagated
					for (auto const spikes : spike_batches) {
						EXPECT_LE(spikes.size(), 1100);
						EXPECT_GE(spikes.size(), 900);
					}
				} else {
					// node does not match input channel, events should not be propagated
					for (auto const spikes : spike_batches) {
						EXPECT_LE(spikes.size(), 100);
					}
				}
			}
		}
	}
}
