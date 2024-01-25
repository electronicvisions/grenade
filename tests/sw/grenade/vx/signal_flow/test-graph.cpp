#include <gtest/gtest.h>

#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include "grenade/vx/signal_flow/vertex/data_input.h"
#include "grenade/vx/signal_flow/vertex/data_output.h"
#include "grenade/vx/signal_flow/vertex/external_input.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view.h"
#include "halco/hicann-dls/vx/v3/event.h"

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>

using namespace halco::hicann_dls::vx::v3;
using namespace grenade::vx::common;
using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex;

TEST(Graph, check_inputs_size)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataInt8, 123);
	auto const v0 = graph.add(vertex, ExecutionInstanceID(), {});

	// too much input vertices
	EXPECT_THROW(graph.add(vertex, ExecutionInstanceID(), {v0}), std::runtime_error);

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::Int8, 123);
	auto const v1 = graph.add(vertex2, ExecutionInstanceID(), {v0});

	// too little input vertices
	EXPECT_THROW(graph.add(vertex2, ExecutionInstanceID(), {}), std::runtime_error);

	// too much input vertices
	EXPECT_THROW(graph.add(vertex2, ExecutionInstanceID(), {v0, v1}), std::runtime_error);
}


TEST(Graph, check_input_port)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataInt8, 123);
	auto const v0 = graph.add(vertex, ExecutionInstanceID(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::Int8, 123);
	auto const v1 = graph.add(vertex2, ExecutionInstanceID(), {v0});

	// wrong port shape
	DataInput vertex3(ConnectionType::Int8, 42);
	EXPECT_THROW(graph.add(vertex3, ExecutionInstanceID(), {v0}), std::runtime_error);

	// invalid port restriction
	EXPECT_THROW(
	    graph.add(vertex3, ExecutionInstanceID(), {{v0, {100, 141} /*size=42 but out-of-range*/}}),
	    std::runtime_error);

	// port restriction not matching input port of vertex (wrong size)
	EXPECT_THROW(
	    graph.add(vertex3, ExecutionInstanceID(), {{v0, {10, 52} /*arbitrary offset, size=43*/}}),
	    std::runtime_error);

	// correct port restriction
	EXPECT_NO_THROW(
	    graph.add(vertex3, ExecutionInstanceID(), {{v0, {10, 51} /*arbitrary offset, size=42*/}}));

	// wrong port type
	EXPECT_THROW(graph.add(vertex2, ExecutionInstanceID(), {v1}), std::runtime_error);
}

TEST(Graph, check_execution_instances)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataInt8, 123);
	auto const v0 = graph.add(vertex, ExecutionInstanceID(), {});

	// too much input vertices
	EXPECT_THROW(graph.add(vertex, ExecutionInstanceID(), {v0}), std::runtime_error);

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::Int8, 123);
	auto const v1 = graph.add(vertex2, ExecutionInstanceID(), {v0});

	// ExternalInput -> DataInput not allowed over different execution instances
	EXPECT_THROW(graph.add(vertex2, ExecutionInstanceID(1), {v0}), std::runtime_error);

	// Graph:: v0 -> v1 -> v2
	DataOutput vertex4(ConnectionType::Int8, 123);
	auto const v2 = graph.add(vertex4, ExecutionInstanceID(), {v1});

	// DataOutput -> DataInput allowed over differrent execution instances
	EXPECT_NO_THROW(graph.add(vertex2, ExecutionInstanceID(1), {v2}));

	// DataOutput -> DataInput only allowed over differrent execution instances
	EXPECT_THROW(graph.add(vertex2, ExecutionInstanceID(), {v2}), std::runtime_error);
}

void test_check_acyclicity(bool enable_check)
{
	Graph graph(enable_check);

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataInt8, 123);
	auto const v0 = graph.add(vertex, ExecutionInstanceID(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::Int8, 123);
	auto const v1 = graph.add(vertex2, ExecutionInstanceID(), {v0});

	// Graph: v0 -> v1 -> v2
	DataOutput vertex4(ConnectionType::Int8, 123);
	auto const v2 = graph.add(vertex4, ExecutionInstanceID(), {v1});

	// Graph: v0 -> v1 -> v2 ==> v3
	auto const v3 = graph.add(vertex2, ExecutionInstanceID(1), {v2});

	// Graph: v0 -> v1 -> v2 ==> v3 -> v4
	auto const v4 = graph.add(vertex4, ExecutionInstanceID(1), {v3});

	// DataOutput -> DataInput back to ExecutionIndex(0) leads to cyclicity
	if (enable_check) {
		EXPECT_THROW(graph.add(vertex2, ExecutionInstanceID(), {v4}), std::runtime_error);
	} else {
		// Note that the graph still won't be executable and the executor will raise the exact same
		// exception.
		EXPECT_NO_THROW(graph.add(vertex2, ExecutionInstanceID(), {v4}));
	}
}

TEST(Graph, check_acyclic_execution_instances)

{
	test_check_acyclicity(true);
	test_check_acyclicity(false);
}


TEST(Graph, check_supports_input_from)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataTimedSpikeToChipSequence, 1);
	auto const v0 = graph.add(vertex, ExecutionInstanceID(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::TimedSpikeToChipSequence, 1);
	auto const v1 = graph.add(vertex2, ExecutionInstanceID(), {v0});

	// Graph: v0 -> v1 -> v2
	CrossbarL2Input vertex3;
	auto const v2 = graph.add(vertex3, ExecutionInstanceID(), {v1});

	// connect loopback via crossbar
	CrossbarNode vertex4(
	    CrossbarNodeOnDLS(
	        SPL1Address().toCrossbarInputOnDLS(),
	        SPL1Address().toCrossbarL2OutputOnDLS().toCrossbarOutputOnDLS()),
	    haldls::vx::v3::CrossbarNode());

	// Graph: v0 -> v1 -> v2 -> v3
	auto const v3 = graph.add(vertex4, ExecutionInstanceID(), {v2});

	// Graph: v0 -> v1 -> v2 -> v3 -> v4
	CrossbarL2Output vertex5;
	auto const v4 = graph.add(vertex5, ExecutionInstanceID(), {v3});

	DataOutput vertex6(ConnectionType::TimedSpikeFromChipSequence, 1);
	EXPECT_NO_THROW(graph.add(vertex6, ExecutionInstanceID(), {v4}));

	// crossbar node not connecting loopback
	CrossbarNode vertex7(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8), PADIBusOnDLS().toCrossbarOutputOnDLS()),
	    haldls::vx::v3::CrossbarNode());

	// Graph: v0 -> v1 -> v5
	auto const v5 = graph.add(vertex7, ExecutionInstanceID(), {v2});

	// Input from crossbar node not connecting to output not supported
	EXPECT_THROW(graph.add(vertex6, ExecutionInstanceID(), {v5}), std::runtime_error);
}

TEST(Graph, recurrence)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataTimedSpikeToChipSequence, 1);
	auto const v0 = graph.add(vertex, ExecutionInstanceID(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::TimedSpikeToChipSequence, 1);
	auto const v1 = graph.add(vertex2, ExecutionInstanceID(), {v0});

	// Graph: v1 -> v2
	CrossbarL2Input vertex3;
	auto const v2 = graph.add(vertex3, ExecutionInstanceID(), {v1});

	CrossbarNode crossbar_in(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8), CrossbarOutputOnDLS(0)),
	    haldls::vx::v3::CrossbarNode());
	auto const v3 = graph.add(crossbar_in, ExecutionInstanceID(), {v2});

	PADIBus::Coordinate c;
	PADIBus padi_bus(c);
	auto const v4 = graph.add(padi_bus, ExecutionInstanceID(), {v3});

	SynapseDriver synapse_driver(
	    SynapseDriver::Coordinate(),
	    {vertex::SynapseDriver::Config::RowAddressCompareMask(0b11111),
	     {vertex::SynapseDriver::Config::RowModes::value_type::excitatory,
	      vertex::SynapseDriver::Config::RowModes::value_type::excitatory},
	     false});
	auto const v5 = graph.add(synapse_driver, ExecutionInstanceID(), {v4});

	SynapseArrayView synapses(
	    SynramOnDLS(), SynapseArrayView::Rows{SynapseRowOnDLS()},
	    SynapseArrayView::Columns{SynapseOnSynapseRow()},
	    SynapseArrayView::Weights{{lola::vx::v3::SynapseMatrix::Weight()}},
	    SynapseArrayView::Labels{{lola::vx::v3::SynapseMatrix::Label()}});
	auto const v6 = graph.add(synapses, ExecutionInstanceID(), {v5});

	NeuronView neurons(
	    NeuronView::Columns{NeuronColumnOnDLS()},
	    NeuronView::Configs{{NeuronView::Config::Label(0), true}}, NeuronRowOnDLS());
	auto const v7 = graph.add(neurons, ExecutionInstanceID(), {v6});

	NeuronEventOutputView neuron_outputs(
	    {{NeuronRowOnDLS(), {NeuronEventOutputView::Columns{NeuronColumnOnDLS()}}}});
	auto const v8 = graph.add(neuron_outputs, ExecutionInstanceID(), {v7});

	// recurrence
	CrossbarNode crossbar_recurrent(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(0), CrossbarOutputOnDLS(0)),
	    haldls::vx::v3::CrossbarNode());
	auto const v9 = graph.add(crossbar_recurrent, ExecutionInstanceID(), {v8});

	auto const v10 = graph.add(v4, ExecutionInstanceID(), {v3, v9});

	auto const v11 = graph.add(v5, ExecutionInstanceID(), {v10});

	auto const v12 = graph.add(v6, ExecutionInstanceID(), {v11});

	auto const v13 = graph.add(v7, ExecutionInstanceID(), {v12});

	auto const v14 = graph.add(v8, ExecutionInstanceID(), {v13});

	CrossbarNode crossbar_out(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(0), CrossbarL2OutputOnDLS().toCrossbarOutputOnDLS()),
	    haldls::vx::v3::CrossbarNode());
	auto const v15 = graph.add(crossbar_out, ExecutionInstanceID(), {v14});

	CrossbarL2Output crossbar_l2_output;
	auto const v16 = graph.add(crossbar_l2_output, ExecutionInstanceID(), {v15});

	DataOutput data_output(ConnectionType::TimedSpikeFromChipSequence, 1);
	graph.add(data_output, ExecutionInstanceID(), {v16});

	// assignment, construction
	Graph graph2(graph); // copy construct
	EXPECT_EQ(graph2, graph);

	Graph graph3;
	graph3 = graph; // copy assign
	EXPECT_EQ(graph2, graph);

	Graph graph4(std::move(graph)); // move construct
	EXPECT_EQ(graph4, graph3);

	Graph graph5;
	graph5 = std::move(graph4); // move assign
	EXPECT_EQ(graph5, graph3);
}

TEST(Graph, CerealizeCoverage)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataTimedSpikeToChipSequence, 1);
	auto const v0 = graph.add(vertex, ExecutionInstanceID(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::TimedSpikeToChipSequence, 1);
	auto const v1 = graph.add(vertex2, ExecutionInstanceID(), {v0});

	// Graph: v1 -> v2
	CrossbarL2Input vertex3;
	auto const v2 = graph.add(vertex3, ExecutionInstanceID(), {v1});

	CrossbarNode crossbar_in(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8), CrossbarOutputOnDLS(0)),
	    haldls::vx::v3::CrossbarNode());
	auto const v3 = graph.add(crossbar_in, ExecutionInstanceID(), {v2});

	PADIBus::Coordinate c;
	PADIBus padi_bus(c);
	auto const v4 = graph.add(padi_bus, ExecutionInstanceID(), {v3});

	SynapseDriver synapse_driver(
	    SynapseDriver::Coordinate(),
	    {vertex::SynapseDriver::Config::RowAddressCompareMask(0b11111),
	     {vertex::SynapseDriver::Config::RowModes::value_type::excitatory,
	      vertex::SynapseDriver::Config::RowModes::value_type::excitatory},
	     false});
	auto const v5 = graph.add(synapse_driver, ExecutionInstanceID(), {v4});

	SynapseArrayView synapses(
	    SynramOnDLS(), SynapseArrayView::Rows{SynapseRowOnDLS()},
	    SynapseArrayView::Columns{SynapseOnSynapseRow()},
	    SynapseArrayView::Weights{{lola::vx::v3::SynapseMatrix::Weight()}},
	    SynapseArrayView::Labels{{lola::vx::v3::SynapseMatrix::Label()}});
	auto const v6 = graph.add(synapses, ExecutionInstanceID(), {v5});

	NeuronView neurons(
	    NeuronView::Columns{NeuronColumnOnDLS()},
	    NeuronView::Configs{{NeuronView::Config::Label(0), true}}, NeuronRowOnDLS());
	auto const v7 = graph.add(neurons, ExecutionInstanceID(), {v6});

	NeuronEventOutputView neuron_outputs(
	    {{NeuronRowOnDLS(), {NeuronEventOutputView::Columns{NeuronColumnOnDLS()}}}});
	auto const v8 = graph.add(neuron_outputs, ExecutionInstanceID(), {v7});

	// recurrence
	CrossbarNode crossbar_recurrent(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(0), CrossbarOutputOnDLS(0)),
	    haldls::vx::CrossbarNode());
	auto const v9 = graph.add(crossbar_recurrent, ExecutionInstanceID(), {v8});

	auto const v10 = graph.add(v4, ExecutionInstanceID(), {v3, v9});

	auto const v11 = graph.add(v5, ExecutionInstanceID(), {v10});

	auto const v12 = graph.add(v6, ExecutionInstanceID(), {v11});

	auto const v13 = graph.add(v7, ExecutionInstanceID(), {v12});

	auto const v14 = graph.add(v8, ExecutionInstanceID(), {v13});

	CrossbarNode crossbar_out(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(0), CrossbarL2OutputOnDLS().toCrossbarOutputOnDLS()),
	    haldls::vx::CrossbarNode());
	auto const v15 = graph.add(crossbar_out, ExecutionInstanceID(), {v14});

	CrossbarL2Output crossbar_l2_output;
	auto const v16 = graph.add(crossbar_l2_output, ExecutionInstanceID(), {v15});

	DataOutput data_output(ConnectionType::TimedSpikeFromChipSequence, 1);
	graph.add(data_output, ExecutionInstanceID(), {v16});

	Graph graph2;

	std::ostringstream ostream;
	{
		cereal::JSONOutputArchive oa(ostream);
		oa(graph);
	}
	std::istringstream istream(ostream.str());
	{
		cereal::JSONInputArchive ia(istream);
		ia(graph2);
	}
	ASSERT_EQ(graph, graph2);
}

TEST(Graph, update)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataTimedSpikeToChipSequence, 1);
	auto const v0 = graph.add(vertex, ExecutionInstanceID(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::TimedSpikeToChipSequence, 1);
	auto const v1 = graph.add(vertex2, ExecutionInstanceID(), {v0});

	// Graph: v0 -> v1 -> v2
	CrossbarL2Input vertex3;
	auto const v2 = graph.add(vertex3, ExecutionInstanceID(), {v1});

	// connect loopback via crossbar
	CrossbarNode vertex4(
	    CrossbarNodeOnDLS(
	        SPL1Address().toCrossbarInputOnDLS(),
	        SPL1Address().toCrossbarL2OutputOnDLS().toCrossbarOutputOnDLS()),
	    haldls::vx::v3::CrossbarNode());

	// Graph: v0 -> v1 -> v2 -> v3
	auto const v3 = graph.add(vertex4, ExecutionInstanceID(), {v2});

	// Graph: v0 -> v1 -> v2 -> v3 -> v4
	CrossbarL2Output vertex5;
	auto const v4 = graph.add(vertex5, ExecutionInstanceID(), {v3});

	DataOutput vertex6(ConnectionType::TimedSpikeFromChipSequence, 1);
	EXPECT_NO_THROW(graph.add(vertex6, ExecutionInstanceID(), {v4}));

	// crossbar node not connecting loopback
	CrossbarNode vertex7(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8), PADIBusOnDLS().toCrossbarOutputOnDLS()),
	    haldls::vx::v3::CrossbarNode());

	// Graph: v0 -> v1 -> X -> v3 -> v4
	EXPECT_THROW(graph.update(v3, vertex7), std::runtime_error);

	// other crossbar node connecting loopback
	CrossbarNode vertex8(
	    CrossbarNodeOnDLS(
	        SPL1Address(1).toCrossbarInputOnDLS(),
	        SPL1Address(1).toCrossbarL2OutputOnDLS().toCrossbarOutputOnDLS()),
	    haldls::vx::v3::CrossbarNode());

	// Graph: v0 -> v1 -> X -> v3 -> v4
	EXPECT_NO_THROW(graph.update(v3, vertex8));
}

TEST(Graph, update_and_relocate)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataTimedSpikeToChipSequence, 1);
	auto const v0 = graph.add(vertex, ExecutionInstanceID(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::TimedSpikeToChipSequence, 1);
	auto const v1 = graph.add(vertex2, ExecutionInstanceID(), {v0});

	// Graph: v1 -> v2
	CrossbarL2Input vertex3;
	auto const v2 = graph.add(vertex3, ExecutionInstanceID(), {v1});

	CrossbarNode crossbar_in(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8), CrossbarOutputOnDLS(0)),
	    haldls::vx::v3::CrossbarNode());
	auto const v3 = graph.add(crossbar_in, ExecutionInstanceID(), {v2});

	CrossbarNode other_crossbar_in(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(9), CrossbarOutputOnDLS(0)),
	    haldls::vx::v3::CrossbarNode());
	auto const other_v3 = graph.add(other_crossbar_in, ExecutionInstanceID(), {v2});

	CrossbarNode other_crossbar_in_different_padi_bus(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8), CrossbarOutputOnDLS(1)),
	    haldls::vx::v3::CrossbarNode());
	auto const other_v3_different_padi_bus =
	    graph.add(other_crossbar_in_different_padi_bus, ExecutionInstanceID(), {v2});

	PADIBus::Coordinate c;
	PADIBus padi_bus(c);
	auto const v4 = graph.add(padi_bus, ExecutionInstanceID(), {v3});

	// working other node
	EXPECT_NO_THROW(graph.update_and_relocate(v4, PADIBus(PADIBus::Coordinate()), {other_v3}));

	// non-working config to node
	EXPECT_THROW(
	    graph.update_and_relocate(v4, PADIBus(PADIBus::Coordinate(halco::common::Enum(1))), {v3}),
	    std::runtime_error);

	// non-working inputs to node
	EXPECT_THROW(
	    graph.update_and_relocate(
	        v4, PADIBus(PADIBus::Coordinate()), {other_v3_different_padi_bus}),
	    std::runtime_error);
}

TEST(Graph, copy)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataTimedSpikeToChipSequence, 1);
	auto const v0 = graph.add(vertex, ExecutionInstanceID(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::TimedSpikeToChipSequence, 1);
	auto const v1 = graph.add(vertex2, ExecutionInstanceID(), {v0});

	// Graph: v1 -> v2
	CrossbarL2Input vertex3;
	auto const v2 = graph.add(vertex3, ExecutionInstanceID(), {v1});

	CrossbarNode crossbar_in(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8), CrossbarOutputOnDLS(0)),
	    haldls::vx::v3::CrossbarNode());
	auto const v3 = graph.add(crossbar_in, ExecutionInstanceID(), {v2});

	Graph graph_copy_construct(graph);
	Graph graph_copy_assign = graph;

	haldls::vx::v3::CrossbarNode config_up;
	config_up.set_mask(haldls::vx::v3::CrossbarNode::neuron_label_type(1));
	CrossbarNode crossbar_up(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8), CrossbarOutputOnDLS(0)), config_up);
	EXPECT_NE(crossbar_in, crossbar_up);
	graph.update(v3, crossbar_up);

	EXPECT_NE(graph.get_vertex_property(v3), graph_copy_construct.get_vertex_property(v3));
	EXPECT_NE(graph.get_vertex_property(v3), graph_copy_assign.get_vertex_property(v3));
	EXPECT_NE(graph, graph_copy_construct);
	EXPECT_NE(graph, graph_copy_assign);

	EXPECT_EQ(graph_copy_assign, graph_copy_construct);
	graph_copy_construct.update(v3, crossbar_up);
	EXPECT_NE(graph_copy_assign, graph_copy_construct);
	EXPECT_EQ(graph, graph_copy_construct);
}
