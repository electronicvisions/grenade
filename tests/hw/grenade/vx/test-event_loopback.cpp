#include <gtest/gtest.h>

#include "grenade/vx/config.h"
#include "grenade/vx/data_map.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/chip.h"
#include "haldls/vx/systime.h"
#include "hxcomm/vx/connection_from_env.h"
#include "logging_ctrl.h"
#include "stadls/vx/init_generator.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/run.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx;
using namespace stadls::vx;
using namespace lola::vx;

grenade::vx::TimedSpikeFromChipSequence test_event_loopback_single_crossbar_node(
    CrossbarL2OutputOnDLS const& node,
    SPL1Address const& address,
    grenade::vx::JITGraphExecutor::ExecutorMap const& executors)
{
	logger_default_config(log4cxx::Level::getTrace());

	grenade::vx::vertex::ExternalInput external_input(
	    grenade::vx::ConnectionType::DataInputUInt16, 1);

	grenade::vx::vertex::DataInput data_input(grenade::vx::ConnectionType::CrossbarInputLabel, 1);

	grenade::vx::vertex::CrossbarNode crossbar_node(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8 + node.toEnum()), node.toCrossbarOutputOnDLS()),
	    haldls::vx::CrossbarNode());

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
	haldls::vx::SpikeLabel label;
	label.set_spl1_address(address);
	grenade::vx::DataMap input_list;
	grenade::vx::TimedSpikeSequence inputs;
	for (intmax_t i = 0; i < 1000; ++i) {
		inputs.push_back(grenade::vx::TimedSpike{
		    grenade::vx::TimedSpike::Time(i * 10),
		    grenade::vx::TimedSpike::Payload(haldls::vx::SpikePack1ToChip({label}))});
	}
	input_list.spike_events[v1] = {inputs};

	grenade::vx::JITGraphExecutor::ConfigMap config_map;
	config_map.insert({DLSGlobal(), grenade::vx::ChipConfig()});

	// run Graph with given inputs and return results
	auto const result_map =
	    grenade::vx::JITGraphExecutor::run(g, input_list, executors, config_map);

	EXPECT_EQ(result_map.spike_event_output.size(), 1);

	EXPECT_TRUE(result_map.spike_event_output.find(v5) != result_map.spike_event_output.end());
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
		stadls::vx::run(executors.at(DLSGlobal()), program);
	}

	for (auto const output : iter_all<CrossbarL2OutputOnDLS>()) {
		for (auto const address : iter_all<SPL1Address>()) {
			auto const spikes =
			    test_event_loopback_single_crossbar_node(output, address, executors);
			if (output.toEnum() == address.toEnum()) {
				// node matches input channel, events should be propagated
				EXPECT_LE(spikes.size(), 1100);
				EXPECT_GE(spikes.size(), 900);
				for (auto const spike : spikes) {
					std::cout << spike.get_chip_time() << std::endl;
				}
			} else {
				// node does not match input channel, events should not be propagated
				EXPECT_LE(spikes.size(), 100);
			}
		}
	}
}
