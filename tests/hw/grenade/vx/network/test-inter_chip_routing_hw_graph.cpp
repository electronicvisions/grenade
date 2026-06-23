#include "grenade/common/edge.h"
#include "grenade/common/input_data.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_input.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include "grenade/vx/signal_flow/vertex/input_routing_table.h"
#include "grenade/vx/signal_flow/vertex/inter_chip_router.h"
#include "grenade/vx/signal_flow/vertex/output_routing_table.h"
#include "helper.h"
#include <gtest/gtest.h>

TEST(InterChipRouting, HwGraphOneToOne)
{
	using namespace grenade::vx::signal_flow::vertex;
	using namespace grenade::vx::network::abstract;

	// Hardware graph
	grenade::common::Topology hardware_graph;

	// Chip identifier
	grenade::vx::common::ChipOnConnection chip_0(0);
	grenade::vx::common::ChipOnConnection chip_1(1);
	grenade::common::TimeDomainOnTopology time_domain;
	grenade::common::ExecutionInstanceOnExecutor execution_instance;

	// Add vertices required for routing
	auto crossbar_input_0 =
	    hardware_graph.add_vertex(CrossbarL2Input(true, chip_0, time_domain, execution_instance));
	auto crossbar_node_0 = hardware_graph.add_vertex(CrossbarNode(
	    halco::hicann_dls::vx::v3::CrossbarNodeOnDLS(halco::common::Y(8), halco::common::X(8)),
	    chip_0, time_domain, execution_instance));
	auto crossbar_output_0 =
	    hardware_graph.add_vertex(CrossbarL2Output(true, chip_0, time_domain, execution_instance));

	auto output_routing_table_0 =
	    hardware_graph.add_vertex(OutputRoutingTable(chip_0, time_domain, execution_instance));
	auto inter_chip_router =
	    hardware_graph.add_vertex(InterChipRouter(time_domain, execution_instance));
	auto input_routing_table_1 =
	    hardware_graph.add_vertex(InputRoutingTable(chip_1, time_domain, execution_instance));

	auto crossbar_input_1 =
	    hardware_graph.add_vertex(CrossbarL2Input(true, chip_1, time_domain, execution_instance));
	auto crossbar_node_1 = hardware_graph.add_vertex(CrossbarNode(
	    halco::hicann_dls::vx::v3::CrossbarNodeOnDLS(halco::common::Y(8), halco::common::X(8)),
	    chip_1, time_domain, execution_instance));
	auto crossbar_output_1 =
	    hardware_graph.add_vertex(CrossbarL2Output(true, chip_1, time_domain, execution_instance));

	// Connect vertices
	hardware_graph.add_edge(
	    crossbar_input_0, crossbar_node_0,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1})));
	hardware_graph.add_edge(
	    crossbar_node_0, crossbar_output_0,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1})));
	hardware_graph.add_edge(
	    crossbar_output_0, output_routing_table_0,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1})));

	hardware_graph.add_edge(
	    output_routing_table_0, inter_chip_router,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1})));
	hardware_graph.add_edge(
	    inter_chip_router, input_routing_table_1,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1})));

	hardware_graph.add_edge(
	    input_routing_table_1, crossbar_input_1,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1})));
	hardware_graph.add_edge(
	    crossbar_input_1, crossbar_node_1,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1})));
	hardware_graph.add_edge(
	    crossbar_node_1, crossbar_output_1,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1})));

	// Routing table contents
	std::map<
	    halco::hicann_dls::vx::v3::OutputRoutingTableEntryOnFPGA,
	    haldls::vx::v3::OutputRoutingTableEntry::Label>
	    output_entries;
	output_entries.emplace(
	    halco::hicann_dls::vx::v3::OutputRoutingTableEntryOnFPGA(13),
	    haldls::vx::v3::OutputRoutingTableEntry::Label(982));

	std::map<
	    halco::hicann_dls::vx::v3::InputRoutingTableEntryOnFPGA,
	    haldls::vx::v3::InputRoutingTableEntry::Label>
	    input_entries;
	input_entries.emplace(
	    halco::hicann_dls::vx::v3::InputRoutingTableEntryOnFPGA(982),
	    haldls::vx::v3::InputRoutingTableEntry::Label(7));

	// Add parameterization of routing tables and crossbar nodes
	grenade::common::InputData input_data;
	input_data.ports.set(
	    {output_routing_table_0, 1}, OutputRoutingTable::Parameterization(output_entries));
	input_data.ports.set(
	    {input_routing_table_1, 1}, InputRoutingTable::Parameterization(input_entries));

	input_data.ports.set(
	    {crossbar_node_0, 1}, CrossbarNode::Parameterization(haldls::vx::v3::CrossbarNode()));
	input_data.ports.set(
	    {crossbar_node_1, 1}, CrossbarNode::Parameterization(haldls::vx::v3::CrossbarNode()));

	// Spike input
	auto spike_label = halco::hicann_dls::vx::v3::SpikeLabel(13);
	haldls::vx::v3::SpikePack1ToChip::labels_type labels;
	labels.fill(spike_label);
	haldls::vx::v3::SpikePack1ToChip spike(labels);

	CrossbarL2Input::Dynamics source_input_data(
	    {{grenade::vx::signal_flow::TimedSpikeToChip(grenade::vx::common::Time(10), spike)}});

	input_data.ports.set({crossbar_input_0, 0}, source_input_data);
	input_data.time_domain_runtimes.set(
	    time_domain, ClockCycleTimeDomainRuntimes(
	                     {grenade::vx::common::Time(
	                         grenade::vx::common::Time::fpga_clock_cycles_per_us *
	                         1000000)},               // 1s just one batch entry
	                     grenade::vx::common::Time()) // no inter batch wait as only one batch entry
	);

	// Execute on hw
	grenade::vx::execution::JITGraphExecutor executor(true, 4);

	// Only execute test if ony jboa setup
	if (!is_jboa_setup_of_size(executor, 4)) {
		GTEST_SKIP() << "Not on Jboa-Multichip Setup";
	}


	auto const results = grenade::vx::execution::run(
	    executor, std::make_shared<grenade::common::Topology>(hardware_graph), input_data);

	// Check results
	auto const source_output_data =
	    dynamic_cast<CrossbarL2Output::Results const&>(results.ports.get({crossbar_output_0, 0}));
	EXPECT_EQ(source_input_data.spikes.size(), source_output_data.spikes.size());
	EXPECT_EQ(source_output_data.spikes.at(0).size(), 1);
	EXPECT_TRUE(
	    source_output_data.spikes.at(0).at(0).data == halco::hicann_dls::vx::v3::SpikeLabel(13));

	auto const target_output_data =
	    dynamic_cast<CrossbarL2Output::Results const&>(results.ports.get({crossbar_output_1, 0}));
	EXPECT_EQ(source_input_data.spikes.size(), target_output_data.spikes.size());
	EXPECT_EQ(target_output_data.spikes.at(0).size(), 1);
	EXPECT_TRUE(
	    target_output_data.spikes.at(0).at(0).data == halco::hicann_dls::vx::v3::SpikeLabel(7));
}